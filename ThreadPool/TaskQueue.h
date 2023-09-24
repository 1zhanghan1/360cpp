#pragma once
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <exception>
#include <string>
#include <functional>
#include <queue>

#include <iostream>

using CallBack = std::function<void()>;

class TaskQueue{
    public:
        TaskQueue() {}
        virtual ~TaskQueue() {}

        virtual bool enqueue(CallBack& task) = 0;
        virtual bool enqueue(CallBack&& task) = 0;
        virtual bool dequeue(CallBack& task) = 0;
        virtual bool empty() = 0;
        virtual int size() = 0;
    private:
        TaskQueue(const TaskQueue& tq) = delete;
        TaskQueue& operator=(const TaskQueue& tq) = delete;
};

/*
    有锁队列，基于std::queue，底层是std::deque
*/
class BlockQueue: public TaskQueue{
    private:
        using LG = std::lock_guard<std::mutex>;
        std::queue<CallBack> taskQueue;
        int maxLen;

        std::mutex mtx;
    
    public:
        BlockQueue(int maxTask)
        : maxLen(maxTask) {}
        ~BlockQueue(){}

        bool enqueue(CallBack& task) override;
        bool enqueue(CallBack&& task) override;
        bool dequeue(CallBack& task) override;

        bool empty() override{
            LG lock(mtx);
            return taskQueue.empty();
        }
        int size() override{
            LG lock(mtx);
            return taskQueue.size();
        }
};

/*
    有锁环形缓冲区，头尾两个互斥量以及必要的原子变量
*/
class BlockRingBuffer: public TaskQueue{
    private:
        using LG = std::lock_guard<std::mutex>;
        std::vector<CallBack> taskQueue;
        int startInd, nextInd, capacity;
        std::atomic<int> len;
        std::mutex startMtx, nextMtx;

    
    public:
        BlockRingBuffer(int maxTask)
        : taskQueue(maxTask), capacity(maxTask), startInd(0), nextInd(0), len(0){}
        ~BlockRingBuffer(){}
        
        bool enqueue(CallBack& task) override;
        bool enqueue(CallBack&& task) override;
        bool dequeue(CallBack& task) override;

        bool empty() override{
            return int(len.load()) == 0;
        }
        int size() override{
            return int(len.load());
        }
};

/*
    无锁队列，基于std::atomic_flag自旋锁
*/
class LockFreeQueue: public TaskQueue{
    private:
        std::queue<CallBack> taskQueue;
        int maxLen;

        std::atomic_flag flag;
    
    public:
        LockFreeQueue(int maxTask)
        : maxLen(maxTask), flag(ATOMIC_FLAG_INIT) {}

        ~LockFreeQueue(){}

        bool enqueue(CallBack& task) override;
        bool enqueue(CallBack&& task) override;
        bool dequeue(CallBack& task) override;

        bool empty() override;
        int size() override;
};

/*
    无锁环形缓冲区，头尾各设置一个flag
*/
class LockFreeRingBuffer: public TaskQueue{
    private:
        std::vector<CallBack> taskQueue;
        std::atomic_flag startFlag, nextFlag;
        int startInd, nextInd, capacity;
        std::atomic<int> len;
    
    public:
        LockFreeRingBuffer(int maxTask)
        : startInd(0), nextInd(0), capacity(maxTask), taskQueue(maxTask), len(0) {}

        ~LockFreeRingBuffer() {}
        
        bool enqueue(CallBack& task) override;
        bool enqueue(CallBack&& task) override;
        bool dequeue(CallBack& task) override;

        bool empty() override{
            return len.load() == 0;
        };
        int size() override{
            return int(len.load());
        };
};

class TaskQueueFullException: public std::exception {
    private:
        std::string message;
    public:
        TaskQueueFullException(): message("Task Queue is full.") {}
        const char* what() const noexcept override {
            return message.c_str();
        }
};


