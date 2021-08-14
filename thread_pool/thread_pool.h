#pragma once

#include <vector>
#include <thread>
#include <coroutine>

#include "queues/mpsc.h"

namespace pt {

namespace thread_pool::detail {
    enum class JobType {
        Coroutine,
        Stop,
    };

    struct Job {
        std::coroutine_handle<> handle;
        JobType type;
    };
}

struct CoroutineThreadPool {
    virtual void push(std::coroutine_handle<> handle) = 0;
};

template<size_t NumThreads>
class FixedCoroutineThreadPool;

template<>
class FixedCoroutineThreadPool<1>: public CoroutineThreadPool {
public:
    FixedCoroutineThreadPool();

    ~FixedCoroutineThreadPool();

    FixedCoroutineThreadPool(const FixedCoroutineThreadPool&) = default;
    FixedCoroutineThreadPool(FixedCoroutineThreadPool&&) = default;

    FixedCoroutineThreadPool& operator=(const FixedCoroutineThreadPool&) = default;
    FixedCoroutineThreadPool& operator=(FixedCoroutineThreadPool&&) = default;

    void push(std::coroutine_handle<> handle) override;

    void stop_and_join();

private:
    void run();

    MpscQueue<thread_pool::detail::Job> jobs;
    std::thread thread;
};


}
