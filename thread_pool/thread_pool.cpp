#include "thread_pool/thread_pool.h"
#include "utils/overload.h"

#include <coroutine>
#include <variant>
#include <algorithm>
#include <thread>
#include <functional>

namespace pt {
using namespace thread_pool::detail;

void FixedCoroutineThreadPool<1>::push(std::coroutine_handle<> handle) {
    jobs.push(JobType::Coroutine{handle});
}

void FixedCoroutineThreadPool<1>::push_sleep_until(std::coroutine_handle<> handle, std::chrono::steady_clock::time_point until) {
    jobs.push(JobType::SleepCoroutine{
        .sleep_until = until,
        .handle = handle,
    });
}

void FixedCoroutineThreadPool<1>::stop_and_join() {
    if (thread.joinable()) {
        jobs.push(JobType::Stop{});

        thread.join();
    }
}

FixedCoroutineThreadPool<1>::FixedCoroutineThreadPool() {
    thread = std::thread(&FixedCoroutineThreadPool::run, this);
}

FixedCoroutineThreadPool<1>::~FixedCoroutineThreadPool() {
    stop_and_join();

    while (!jobs.empty()) {
        Job j = jobs.pop();
        std::visit(
            overload{
                [](JobType::Stop){},
                [](JobType::SleepCoroutine& c) {
                    if (c.handle) c.handle.destroy();
                },
                [](JobType::Coroutine& c) {
                    if (c.handle) c.handle.destroy();
                }
            },
            j
        );
    }

    for (auto& c: sleeping_coroutines) {
        if (c.handle) {
            c.handle.destroy();
        }
    }
}

void FixedCoroutineThreadPool<1>::run() {
    while (true) {
        Job job;

        if (sleeping_coroutines.empty()) {
            job = jobs.pop();
        } else {
            auto maybe_job = jobs.wait_until(sleeping_coroutines.front().sleep_until);
            if (maybe_job.has_value()) {
                job = *maybe_job;
            } else {
                std::pop_heap(
                    sleeping_coroutines.begin(),
                    sleeping_coroutines.end(),
                    std::greater<JobType::SleepCoroutine>{}
                );
                auto c = sleeping_coroutines.back();
                sleeping_coroutines.pop_back();
                c.handle.resume();
                continue;
            }
        }

        bool stop = false;
        std::visit(
            overload{
                [&](JobType::Stop) {stop = true;},
                [](JobType::Coroutine& c) {c.handle.resume();},
                [&](JobType::SleepCoroutine& c) {
                    if (c.sleep_until < std::chrono::steady_clock::now()) {
                        c.handle.resume();
                    } else {
                        sleeping_coroutines.push_back(c);
                        std::push_heap(
                            sleeping_coroutines.begin(),
                            sleeping_coroutines.end(),
                            std::greater<JobType::SleepCoroutine>{}
                        );
                    }
                },
            },
            job
        );
        if (stop) return;
    }
}

}