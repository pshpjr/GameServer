//
// Created by pshpj on 24. 10. 17.
//

#include "ThreadPool.h"

ThreadPool::ThreadPool(size_t threads)
    : stop(false)
{
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back(
            [this] {
                while (true)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock lock(this->queue_mutex);
                        this->condition.wait(lock,
                                             [this] {
                                                 return this->stop || !this->tasks.empty();
                                             });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            }
        );
}


ThreadPool::~ThreadPool()
{
    {
        std::unique_lock lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
}

void ThreadPool::enqueue(const std::function<void()>& func)
{
    std::unique_lock lock(queue_mutex);
    if (stop)
    {
        throw std::runtime_error("stopped ThreadPool");
    }

    tasks.emplace(func);

    condition.notify_one();
}
