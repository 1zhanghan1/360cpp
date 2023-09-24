#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <functional>
#include <thread>
#include <future>
#include <string>
#include <chrono>
#include <iostream>

#include <execinfo.h>
#include <dlfcn.h>

#include "TaskQueue.h"

enum class TaskQueueType{
    BLOCK_QUEUE,
    BLOCK_RINGBUFFER,
    LOCKFREE_QUEUE,
    LOCKFREE_RINGBUFFER
};

//核心线程的创建时机
enum class InitType{
    LAZY,
    HUNGER
};
//任务队列满后的操作
enum class FullOperate{
    REJECT,
    EXCEPTION
};
enum class TheadPoolType{
    PLAIN,
    COMPOSITE
};


// PLAIN
class ThreadPool{
    protected:
        
        std::unique_ptr<TaskQueue> taskQueuePtr, urgTaskQueuePtr;
        std::vector<std::thread> threads;
        volatile std::atomic<bool> isShutDown;
        volatile std::atomic<int> blockedThreads;
        int size, minSize, maxSize, busyThred, freeThred;
        int freeId;

        std::mutex mtxOfTaskQueuePtr, mtxOfThreads;
        std::condition_variable cvOfTaskQueuePtr;

        InitType initType;
        TaskQueueType tqType;
        FullOperate fullOperate;
        TheadPoolType threadPoolType;

        //内部类
        class ThreadWork{
            private:
                ThreadPool* pool;
                int tid;
            public:
                ThreadWork(ThreadPool* _pool, int id);
                void operator()();
        };
        ThreadPool() = delete;
    public:
        ThreadPool(int minThreads, int maxThreads = 0, int maxQueueLen = 500, int busyThreshold = 0,
         int freeThreshold = 0, InitType it = InitType::HUNGER, TaskQueueType tt = TaskQueueType::LOCKFREE_RINGBUFFER, FullOperate fo = FullOperate::REJECT);
        ~ThreadPool();
        
        virtual void start();

        void shutdown();

        template<typename F, typename... Args>
        auto submit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>{
            auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            
            // 使用智能指针防止局部变量释放导致内存泄漏
            auto taskPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
            
            
            CallBack callBack = [taskPtr](){
                try{
                    (*taskPtr)();
                }catch (std::exception& e){
                    //LOG e.what()
                    std::cerr << e.what() << std::endl;
                }

            };


            //懒加载线程池，没有线程被阻塞且线程数没达到minSize时 增加线程
            if(initType == InitType::LAZY && (int)(blockedThreads.load()) == 0){
                std::lock_guard<std::mutex> lock(mtxOfThreads);
                if(size < minSize){
                    threads.push_back( std::move(std::thread(ThreadWork(this, size))) );
                    ++size;
                    std::cout << "LAZY" << std::endl;
                }
            }
            
            //入队，一次性
            bool res = taskQueuePtr->enqueue(std::move(callBack));
            if(res){
                cvOfTaskQueuePtr.notify_one();
                // std::cout << "submit one" << std::endl;
                return taskPtr->get_future();
            }

            if(fullOperate == FullOperate::EXCEPTION){
                throw TaskQueueFullException();
            }
            // fullOperate == FullOperate::REJECT
            // std::cout << "FullOperate::REJECT" << std::endl;
            return std::future<decltype(f(args...))>();
        }
    private:
        void dynamicScale(){
            while(1){
                // std::cout << "dynamicScale" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                {
                    std::lock_guard<std::mutex> lock(mtxOfThreads);
                    if(size < minSize) continue;
                    
                    //空闲
                    if(size > minSize && blockedThreads.load() > freeThred){
                        freeId = size-1;
                        
                        cvOfTaskQueuePtr.notify_all();
                        if(threads[size-1].joinable()){
                            threads[size-1].join();
                        }
                        
                        freeId = -1;
                        --size;
                        threads.resize(size);
                        std::cout << "size-1" << std::endl;
                    }else if(size < maxSize && taskQueuePtr->size() > busyThred){  //忙碌
                        threads.push_back( std::move(std::thread(ThreadWork(this, size))) );
                        ++size;
                        std::cout << "size+1" << std::endl;
                    }
                }
            }
        }
};




class ComposeThreadPool: public ThreadPool{
    public:
        ComposeThreadPool(int maxUrgTask, int minThreads, int maxThreads = 0, int maxQueueLen = 500, int busyThreshold = 0,
         int freeThreshold = 0, InitType it = InitType::HUNGER, TaskQueueType tt = TaskQueueType::LOCKFREE_RINGBUFFER, FullOperate fo = FullOperate::REJECT);
        
        
        template<typename F, typename... Args>
        auto urgSubmit(F&& f, Args&&... args) -> std::future<decltype(f(args...))>{
            auto func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
            
            // 使用智能指针防止局部变量释放导致内存泄漏
            auto taskPtr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);
            
            
            CallBack callBack = [taskPtr](){
                try{
                    (*taskPtr)();
                }catch (std::exception& e){
                    //LOG e.what()
                    std::cerr << e.what() << std::endl;
                }

            };
            
            
            //入队，同步
            while(!urgTaskQueuePtr->enqueue(std::move(callBack))) {}
            // 不需要唤醒线程
            // cvOfTaskQueuePtr.notify_one();
            // std::cout << "submit one" << std::endl;
            return taskPtr->get_future();
        }
};








