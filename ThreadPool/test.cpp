#include <iostream>
#include <chrono>
#include <vector>
#include <random>
#include <ctime>

#include "ThreadPool.h"

using namespace std;

void FuncSleep(int ms){
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void FuncLittle(){
    //do something
    FuncSleep(10);
}
void FuncBig(){
    //do something
    FuncSleep(100);
}
void FuncLarge(){
    //do something
    FuncSleep(500);
}
int LastFunc(){
    return 1;
}

//全小任务
void CaseofAllLittle(ThreadPool& pool, int n, bool wait = true){
    for(int i = 0; i < n; ++i){
        pool.submit(FuncLittle);
    }
    if(wait){
        auto res = pool.submit(LastFunc);
        if(res.valid()){
            int x = res.get();
            std::cout << "CaseofAllLittle finish.";
        }else{
            std::cout << "CaseofAllLittle Full";
        }
    }
}
//全大任务
void CaseofAllBig(ThreadPool& pool, int n, bool wait = true){
    for(int i = 0; i < n; ++i){
        pool.submit(FuncBig);
    }
    if(wait){
        auto res = pool.submit(LastFunc);
        if(res.valid()){
            int x = res.get();
            std::cout << "CaseofAllBig finish.";
        }else{
            std::cout << "CaseofAllBig Full";
        }
    }
}
//全超大任务
void CaseofAllLarge(ThreadPool& pool, int n, bool wait = true){
    for(int i = 0; i < n; ++i){
        pool.submit(FuncLarge);
    }
    if(wait){
        auto res = pool.submit(LastFunc);
        if(res.valid()){
            int x = res.get();
            std::cout << "CaseofAllLarge finish.";
        }else{
            std::cout << "CaseofAllLarge Full";
        }
    }
}
//little,big,large混合
void CaseofHybird(ThreadPool& pool, int n, bool wait = true){
    // 一半是little，四分之一是big，四分之一是large
    vector<std::function<void()>> testFunc;
    testFunc.reserve(n);
    for(int i = 0; i < n; ++i){
        if(i < (n>>1)){
            testFunc.push_back(FuncLittle);
        }else if(i < ((n>>1) + (n>>2))){
            testFunc.push_back(FuncBig);
        }else{
            testFunc.push_back(FuncLarge);
        }
    }
    std::mt19937 generater(time(0));

    while(n){
        std::uniform_int_distribution<> dis(0, n-1);
        int randomInd = dis(generater);
        pool.submit(testFunc[randomInd]);
        swap(testFunc[randomInd], testFunc[n-1]);
        --n;
    }
    if(wait){
        auto res = pool.submit(LastFunc);
        if(res.valid()){
            int x = res.get();
            std::cout << "CaseofHybird finish.";
        }else{
            std::cout << "CaseofHybird Full";
        }
    }
}


void SingleThreadTest(ThreadPool& pool, int n){
    auto start = std::chrono::high_resolution_clock::now();
    CaseofAllLittle(pool, n);
    auto end = std::chrono::high_resolution_clock::now();
    int us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;

    start = std::chrono::high_resolution_clock::now();
    CaseofAllBig(pool, n);
    end = std::chrono::high_resolution_clock::now();
    us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;

    start = std::chrono::high_resolution_clock::now();
    CaseofAllLarge(pool, n);
    end = std::chrono::high_resolution_clock::now();
    us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;

    start = std::chrono::high_resolution_clock::now();
    CaseofHybird(pool, n);
    end = std::chrono::high_resolution_clock::now();
    us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;
}


void MultiThreadLittle(ThreadPool& pool, int n){
    vector<thread> threads;
    for(int i = 0; i < 4; ++i){
        threads.push_back(thread(CaseofAllLittle, ref(pool), n>>2, false));
    }
    for(int i = 0; i < 4; ++i){
        threads[i].join();
    }
    auto res = pool.submit(LastFunc);
    if(res.valid()){
        int x = res.get();
        std::cout << "MultiThreadLittle finish.";
    }else{
        std::cout << "MultiThreadLittle Full";
    }
}
void MultiThreadBig(ThreadPool& pool, int n){
    vector<thread> threads;
    for(int i = 0; i < 4; ++i){
        threads.push_back(thread(CaseofAllBig, ref(pool), n>>2, false));
    }
    for(int i = 0; i < 4; ++i){
        threads[i].join();
    }
    auto res = pool.submit(LastFunc);
    if(res.valid()){
        int x = res.get();
        std::cout << "MultiThreadBig finish.";
    }else{
        std::cout << "MultiThreadBig Full";
    }
}
void MultiThreadLarge(ThreadPool& pool, int n){
    vector<thread> threads;
    for(int i = 0; i < 4; ++i){
        threads.push_back(thread(CaseofAllLarge, ref(pool), n>>2, false));
    }
    for(int i = 0; i < 4; ++i){
        threads[i].join();
    }
    auto res = pool.submit(LastFunc);
    if(res.valid()){
        int x = res.get();
        std::cout << "MultiThreadLarge finish.";
    }else{
        std::cout << "MultiThreadLarge Full";
    }
}
void MultiThreadHybird(ThreadPool& pool, int n){
    vector<thread> threads;
    for(int i = 0; i < 4; ++i){
        threads.push_back(thread(CaseofHybird, ref(pool), n>>2, false));
    }
    for(int i = 0; i < 4; ++i){
        threads[i].join();
    }
    auto res = pool.submit(LastFunc);
    if(res.valid()){
        int x = res.get();
        std::cout << "MultiThreadHybird finish.";
    }else{
        std::cout << "MultiThreadHybird Full";
    }
}

void MultiThreadTest(ThreadPool& pool, int n){
    auto start = std::chrono::high_resolution_clock::now();
    MultiThreadLittle(pool, n);
    auto end = std::chrono::high_resolution_clock::now();
    int us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;
    
    start = std::chrono::high_resolution_clock::now();
    MultiThreadBig(pool, n);
    end = std::chrono::high_resolution_clock::now();
    us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;
    
    start = std::chrono::high_resolution_clock::now();
    MultiThreadLarge(pool, n);
    end = std::chrono::high_resolution_clock::now();
    us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;
    
    start = std::chrono::high_resolution_clock::now();
    MultiThreadHybird(pool, n);
    end = std::chrono::high_resolution_clock::now();
    us = std::chrono::duration_cast<std::chrono::microseconds>(end-start).count();
    std::cout << "cost time: " << us/1000.0 << std::endl;
}

void TestFunc(int x){
    FuncSleep(10);
    cout << x << endl;
}
int TestFuncR(int x){
    return x+1;
}

void FunctionalTest(){
    {// 普通测试
        cout << "first" << endl;
        ThreadPool pool(4);
        pool.start();
        for(int i = 0; i < 10; ++i){
            pool.submit(TestFunc, i);
        }
        auto res = pool.submit(TestFuncR, 10);
        if(res.valid()){
            cout << res.get() << " get return." << endl;
        }
    }
    {// 队列满拒绝
        cout << "second" << endl;
        ThreadPool pool(1, 0, 5);
        pool.start();
        for(int i = 0; i < 6; ++i){
            pool.submit(TestFunc, i);
        }
        auto res = pool.submit(TestFunc, 10);
        if(!res.valid()){
            cout << "reject" << endl;
        }
    }
    {// 队列满异常
        cout << "third" << endl;
        ThreadPool pool(1, 0, 5, 0, 0, InitType::HUNGER, TaskQueueType::LOCKFREE_RINGBUFFER, FullOperate::EXCEPTION);
        pool.start();
        for(int i = 0; i < 10; ++i){
            try{
                pool.submit(TestFunc, i);
            }catch (exception& e){
                cout << "exception " << e.what() << endl;
            }
        }
    }
    {// 懒加载
        cout << "fourth" << endl;
        ThreadPool pool(3,0,100, 0, 0, InitType::LAZY);
        pool.start();
        for(int i = 0; i < 10; ++i){
            pool.submit(TestFunc, i);
        }
    }
    {// 动态放缩
        cout << "fifth" << endl;
        ThreadPool pool(1, 3, 100, 3, 1);
        pool.start();
        for(int i = 0; i < 99; ++i){
            pool.submit(FuncSleep, 100);
        }
        FuncSleep(20000);
    }
}


int main(){

    // FunctionalTest();
    // ThreadPool pool(4, 0, 1100, 0, 0, InitType::HUNGER, TaskQueueType::LOCKFREE_RINGBUFFER);
    // pool.start();

    // SingleThreadTest(pool, 1024);
    // cout << endl;
    // MultiThreadTest(pool, 1024);

    // pool.shutdown();
    ComposeThreadPool pool(10, 4);
    pool.start();

    auto res = pool.urgSubmit(LastFunc);
    cout << res.get() << endl;

    pool.shutdown();
    return 0;
}
