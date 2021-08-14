#include "thread_pool/thread_pool.h"

#include <coroutine>
#include <thread>

namespace pt {
using namespace thread_pool::detail;

void FixedCoroutineThreadPool<1>::push(std::coroutine_handle<> handle) {
    jobs.push(Job{
        .handle = handle,
        .type = JobType::Coroutine,
    });
}

void FixedCoroutineThreadPool<1>::stop_and_join() {
    if (thread.joinable()) {
        jobs.push(Job{
            .handle = nullptr,
            .type = JobType::Stop,
        });

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
        switch (j.type) {
            case JobType::Stop: {
                break;
            }
            case JobType::Coroutine: {
                j.handle.destroy();
            }
        }
    }
}

void FixedCoroutineThreadPool<1>::run() {
    while (true) {
        Job job = jobs.pop();
        switch (job.type) {
            case JobType::Coroutine: {
                job.handle.resume();
                break;
            }
            case JobType::Stop: {
                return;
            }
        }
    }
}

}