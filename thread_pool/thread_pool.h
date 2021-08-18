#pragma once

#include <vector>
#include <thread>
#include <coroutine>
#include <chrono>
#include <set>
#include <variant>
#include <compare>

#include "queues/mpsc.h"


namespace pt {

namespace thread_pool::detail {
    namespace JobType {
        struct Coroutine {
            std::coroutine_handle<> handle;
        };

        struct SleepCoroutine {
            auto operator<=>(const SleepCoroutine&) const = default;
            std::chrono::steady_clock::time_point sleep_until;
            std::coroutine_handle<> handle;
        };

        struct Stop {};
    };

    using Job = std::variant<JobType::Coroutine, JobType::SleepCoroutine, JobType::Stop>;
}

struct CoroutineThreadPool {
    virtual void push(std::coroutine_handle<> handle) = 0;
    virtual void push_sleep_until(std::coroutine_handle<> handle, std::chrono::steady_clock::time_point until) = 0;

    template<typename Rep, typename Period>
    void push_sleep(std::coroutine_handle<> handle, std::chrono::duration<Rep, Period> duration) {
        push_sleep_for(handle, std::chrono::steady_clock::now() + duration);
    }
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
    void push_sleep_until(std::coroutine_handle<> handle, std::chrono::steady_clock::time_point until) override;

    void stop_and_join();

private:
    void run();

    MpscQueue<thread_pool::detail::Job> jobs;

    // maintained by push_heap and pop_heap
    std::vector<thread_pool::detail::JobType::SleepCoroutine> sleeping_coroutines;
    std::thread thread;
};


}
