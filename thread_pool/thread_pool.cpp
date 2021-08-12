#include "thread_pool/thread_pool.h"

#include <coroutine>
#include <thread>
#include <iostream>

namespace pt {
using namespace thread_pool::detail;

void FixedCoroutineThreadPool<1>::push(std::coroutine_handle<> handle) {
    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    jobs.push(Job{
        .handle = handle,
        .type = JobType::Coroutine,
    });
}

void FixedCoroutineThreadPool<1>::stop_and_join() {
    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    if (thread.joinable()) {
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        jobs.push(Job{
            .handle = nullptr,
            .type = JobType::Stop,
        });

        thread.join();
    }
}

FixedCoroutineThreadPool<1>::FixedCoroutineThreadPool() {
    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    thread = std::thread(&FixedCoroutineThreadPool::run, this);
}

FixedCoroutineThreadPool<1>::~FixedCoroutineThreadPool() {
    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    while (!jobs.empty()) {
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        Job j = jobs.pop();
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        switch (j.type) {
            case JobType::Stop: {
                break;
            }
            case JobType::Coroutine: {
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                j.handle.destroy();
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            }
        }
    }
}

void FixedCoroutineThreadPool<1>::run() {
    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    while (true) {
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        Job job = jobs.pop();
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        switch (job.type) {
            case JobType::Coroutine: {
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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