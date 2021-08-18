#pragma once

#include <chrono>

#include "thread_pool/promise.h"

namespace pt {

struct sleep_until {
    sleep_until(std::chrono::steady_clock::time_point until): until(until) {}

    bool await_ready() {
        return until < std::chrono::steady_clock::now();
    }

    template<typename U>
    void await_suspend(std::coroutine_handle<U> h) noexcept {
        h.promise().pool->push_sleep_until(h, until);
    }

    void await_resume() {}

    std::chrono::steady_clock::time_point until;
};

template<>
struct AwaitTransformPassThrough<sleep_until> {
    static constexpr bool pass_through = true;
};

}