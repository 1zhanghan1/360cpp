#include "ThreadPool.h"

ThreadPool::ThreadWork::ThreadWork(ThreadPool* _pool, int id): pool(_pool), tid(id) {}

void ThreadPool::ThreadWork::operator()(){
    CallBack func;
    bool dequeued = false;
    while(!pool->isShutDown.load()){
        //释放
        if(pool->freeId == tid) break;
        
        //线程0优先执行urgTaskQueuePtr
        if(tid == 0 && pool->threadPoolType == TheadPoolType::COMPOSITE){
            while(!pool->urgTaskQueuePtr->empty()){
                dequeued = pool->urgTaskQueuePtr->dequeue(func);
                if(dequeued){
                    func();
                }
            }
        }

        {
            // std::cout << "2" << std::endl;
            std::unique_lock<std::mutex> lock(pool->mtxOfTaskQueuePtr);
            if(pool->taskQueuePtr->empty()){
                pool->blockedThreads.fetch_add(1);
                if(tid != 0){
                    pool->cvOfTaskQueuePtr.wait(lock);
                }
                else{
                    if(pool->cvOfTaskQueuePtr.wait_for(lock, std::chrono::milliseconds(100)) == std::cv_status::timeout){
                        continue;
                    }
                }
                pool->blockedThreads.fetch_sub(1);
            }
            // std::cout << "3" << std::endl;
            dequeued = pool->taskQueuePtr->dequeue(func);
        }

        if(dequeued){
            // std::cout << "run one" << std::endl;
            func();
        }
        // std::cout << pool->isShutDown.load() << std::endl;
    }
}

inline int max(int a, int b){
    return a > b ? a : b;
}

ThreadPool::ThreadPool(int minThreads, int maxThreads, int maxQueueLen, int busyThreshold,
 int freeThreshold, InitType it, TaskQueueType tt, FullOperate fo)
:size(0), minSize(minThreads), maxSize(maxThreads), busyThred(busyThreshold), 
freeThred(freeThreshold), initType(it), tqType(tt), fullOperate(fo),
 isShutDown(true), blockedThreads(0), freeId(-1), threads(), taskQueuePtr(nullptr), urgTaskQueuePtr(nullptr) 
{
    minSize = max(1, minSize);

    threads.reserve(maxThreads);
    
    threadPoolType = TheadPoolType::PLAIN;

    switch (tqType)
    {
        case TaskQueueType::BLOCK_QUEUE:
            taskQueuePtr.reset(new BlockQueue(maxQueueLen));
            break;
        case TaskQueueType::BLOCK_RINGBUFFER:
            taskQueuePtr.reset(new BlockRingBuffer(maxQueueLen));
            break;
        case TaskQueueType::LOCKFREE_QUEUE:
            taskQueuePtr.reset(new LockFreeQueue(maxQueueLen));
            break;
        case TaskQueueType::LOCKFREE_RINGBUFFER:
            taskQueuePtr.reset(new LockFreeRingBuffer(maxQueueLen));
            break;
    }
    if(maxSize > minSize){
        if(busyThreshold == 0){
            busyThreshold = maxQueueLen >> 1;
        }
        if(freeThreshold == 0){
            freeThreshold = minSize >> 1;
        }
    }
}

void ThreadPool::start(){
    isShutDown.store(false);
    if(initType == InitType::HUNGER){
        for(int i = 0; i < minSize; ++i){
            threads.push_back( std::move(std::thread(ThreadWork(this, i))) );
        }
        size = minSize;
        // std::cout << "1" << std::endl;
    }
    if(maxSize > minSize){
        //动态调整
        std::cout << "create detached thread" << std::endl;
        std::thread dynamicScaler(std::bind(&ThreadPool::dynamicScale, this));
        dynamicScaler.detach();
    }
}

void ThreadPool::shutdown(){
    int t = 100;
    while(!taskQueuePtr->empty()){
        std::this_thread::sleep_for(std::chrono::milliseconds(t));
        t <<= 1;
    }
    isShutDown.store(true);
    cvOfTaskQueuePtr.notify_all();
    
    for(int i = 0; i < size; ++i){
        if(threads.at(i).joinable()){
            threads.at(i).join();
        }
    }
    size = 0;
    threads.resize(size);
}

ThreadPool::~ThreadPool(){
    if(!isShutDown.load())
        shutdown();
    // std::cout << "~ThreadPool" << std::endl;
}



ComposeThreadPool::ComposeThreadPool(int maxUrgTask, int minThreads, int maxThreads, int maxQueueLen, int busyThreshold,
 int freeThreshold, InitType it, TaskQueueType tt, FullOperate fo)
 : ThreadPool(minThreads, maxThreads, maxQueueLen, busyThreshold, freeThreshold, it, tt, fo)
{
    threadPoolType = TheadPoolType::COMPOSITE;
    maxUrgTask = max(1, maxUrgTask);
    urgTaskQueuePtr.reset(new BlockRingBuffer(maxUrgTask));
}





