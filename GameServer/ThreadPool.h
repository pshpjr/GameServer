//
// Created by pshpj on 24. 10. 17.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H


#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>


class ThreadPool
{
public:
    ThreadPool(size_t threads);
    ~ThreadPool();
    void enqueue(const std::function<void()>& func);

private:
    std::queue<std::function<void()>> tasks;
    std::vector<std::jthread> workers;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};


#endif //THREADPOOL_H
