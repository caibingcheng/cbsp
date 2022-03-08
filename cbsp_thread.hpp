#ifndef _CBSP_THREAD_H_
#define _CBSP_THREAD_H_

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

namespace cbsp
{
    class ThreadPoolBase
    {
    public:
        ThreadPoolBase(size_t);
        template <class F, class... Args>
        auto enqueue(F &&f, Args &&...args)
            -> std::future<typename std::result_of<F(Args...)>::type>;
        void wait();
        ~ThreadPoolBase();

    private:
        // need to keep track of threads so we can join them
        std::vector<std::thread> workers;
        // the task queue
        std::queue<std::function<void()>> tasks;

        // synchronization
        std::mutex queue_mutex;
        std::condition_variable condition;
        bool stop;
    };

    // the constructor just launches some amount of workers
    inline ThreadPoolBase::ThreadPoolBase(size_t threads)
        : stop(false)
    {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                [this]
                {
                    for (;;)
                    {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                                 [this]
                                                 { return this->stop || !this->tasks.empty(); });
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }

                        task();
                    }
                });
    }

    // add new work item to the pool
    template <class F, class... Args>
    auto ThreadPoolBase::enqueue(F &&f, Args &&...args)
        -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            // don't allow enqueueing after stopping the pool
            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPoolBase");

            tasks.emplace([task]()
                          { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    inline void ThreadPoolBase::wait()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            if (worker.joinable())
                worker.join();
    }

    // the destructor joins all threads
    inline ThreadPoolBase::~ThreadPoolBase()
    {
        wait();
    }

    class ThreadPool
    {
    public:
        ~ThreadPool() = default;
        static void create(size_t threads)
        {
            if (!threadpool)
            {
                threadpool = std::make_shared<ThreadPoolBase>(threads);
            }
        }

        template <class F, class... Args>
        static auto enqueue(F &&f, Args &&...args)
            -> std::future<typename std::result_of<F(Args...)>::type>
        {
            if (threadpool)
                return threadpool->enqueue(f, args...);
            throw std::runtime_error("null threadpool");
        }

        static void wait()
        {
            if (threadpool)
                threadpool->wait();
            else
                throw std::runtime_error("null threadpool");
        }

    private:
        ThreadPool() = delete;
        static std::shared_ptr<ThreadPoolBase> threadpool;
    };
    std::shared_ptr<ThreadPoolBase> ThreadPool::threadpool = nullptr;
}

#endif