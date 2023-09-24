#include "TaskQueue.h"
#include <atomic>


/*
    有锁队列，基于std::queue，底层是std::deque
*/

bool BlockQueue::enqueue(CallBack& task){
    LG lock(mtx);
    if(taskQueue.size() == maxLen) return false;

    taskQueue.push(task);
    return true;
}
bool BlockQueue::enqueue(CallBack&& task){
    LG lock(mtx);
    if(taskQueue.size() == maxLen) return false;

    taskQueue.push(std::move(task));
    return true;
}
bool BlockQueue::dequeue(CallBack& task){
    LG lock(mtx);
    if(taskQueue.empty()) return false;

    task = taskQueue.front();
    taskQueue.pop();
    return true;
}


/*
    有锁环形缓冲区，头尾两个互斥量以及必要的原子变量
*/

bool BlockRingBuffer::enqueue(CallBack& task){
    if(int(len.load()) == capacity) return false;
    LG lock(nextMtx);

    taskQueue[nextInd++] = task;
    if(nextInd == capacity){
        nextInd = 0;
    }
    len.fetch_add(1);
    return true;
}


bool BlockRingBuffer::enqueue(CallBack&& task){
    if(int(len.load()) == capacity) return false;
    LG lock(nextMtx);

    taskQueue[nextInd++] = std::move(task);
    if(nextInd == capacity){
        nextInd = 0;
    }
    len.fetch_add(1);
    return true;
}


bool BlockRingBuffer::dequeue(CallBack& task){
    // std::cout << "want dequeue one" << std::endl;
    if(int(len.load()) == 0) return false;
    LG lock(startMtx);

    task = taskQueue[startInd++];
    if(startInd == capacity){
        startInd = 0;
    }
    len.fetch_sub(1);
    return true;
}


/*
    无锁队列，基于std::atomic_flag自旋锁
*/

bool LockFreeQueue::enqueue(CallBack& task){
    bool res = false;

    while(flag.test_and_set(std::memory_order_acquire)) {}
    res = taskQueue.size() < maxLen;
    if(res){
        taskQueue.push(task);
    }
    flag.clear(std::memory_order_release);

    return res;
}
bool LockFreeQueue::enqueue(CallBack&& task){
    bool res = false;

    while(flag.test_and_set(std::memory_order_acquire)) {}
    res = taskQueue.size() < maxLen;
    if(res){
        taskQueue.push(std::move(task));
    }
    flag.clear(std::memory_order_release);

    return res;
}

bool LockFreeQueue::dequeue(CallBack& task){
    bool res = false;

    while(flag.test_and_set(std::memory_order_acquire)) {}
    res = !taskQueue.empty();
    if(res){
        task = taskQueue.front();
        taskQueue.pop();
    }
    flag.clear(std::memory_order_release);

    return res;
}

bool LockFreeQueue::empty(){
    bool res = false;

    while(flag.test_and_set(std::memory_order_acquire)) {}
    res = taskQueue.empty();
    flag.clear(std::memory_order_release);

    return res;
}

int LockFreeQueue::size(){
    int res = 0;
    
    while(flag.test_and_set(std::memory_order_acquire)) {}
    res = taskQueue.size();
    flag.clear(std::memory_order_release);
    
    return res;
}


/*
    无锁环形缓冲区，头尾各设置一个flag
*/

bool LockFreeRingBuffer::enqueue(CallBack& task){
    bool res = false;

    while(nextFlag.test_and_set(std::memory_order_acquire)) {}
    res = len.load() < capacity;
    if(res){
        taskQueue[nextInd++] = task;
        if(nextInd == capacity){
            nextInd = 0;
        }
        len.fetch_add(1);
    }
    nextFlag.clear(std::memory_order_release);

    return res;
}

bool LockFreeRingBuffer::enqueue(CallBack&& task){
    bool res = false;

    while(nextFlag.test_and_set(std::memory_order_acquire)) {}
    res = len.load() < capacity;
    if(res){
        taskQueue[nextInd++] = std::move(task);
        if(nextInd == capacity){
            nextInd = 0;
        }
        len.fetch_add(1);
    }
    nextFlag.clear(std::memory_order_release);

    return res;
}

bool LockFreeRingBuffer::dequeue(CallBack& task){
    bool res = false;

    while(startFlag.test_and_set(std::memory_order_acquire)) {}
    res = len.load() > 0;
    if(res){
        task = taskQueue[startInd++];
        if(startInd == capacity){
            startInd = 0;
        }
        len.fetch_sub(1);
    }
    startFlag.clear(std::memory_order_release);

    return res;
}





