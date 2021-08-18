#pragma once
#include <coroutine>
#include <deque>
#include <cassert>

#include "thread_pool/promise.h"
#include "thread_pool/thread_pool.h"

namespace pt {

class SingleThreadedMutex;


class [[nodiscard]] MutexGuard {
public:
    ~MutexGuard();

    MutexGuard(const MutexGuard&) = delete;
    MutexGuard(MutexGuard&&) = delete;

    MutexGuard& operator=(const MutexGuard&) = delete;
    MutexGuard& operator=(MutexGuard&&) = delete;
private:
    MutexGuard(SingleThreadedMutex* m): m(m) {}
    SingleThreadedMutex* m;

    friend class SingleThreadedMutex;
};

class SingleThreadedMutex {
public:
    class awaiter {
    public:
        bool await_ready();

        template<typename U>
        void await_suspend(std::coroutine_handle<U> h) noexcept {
            m->pool = h.promise().pool;
            m->waiting.push_back(h);
        }

        MutexGuard await_resume();

        awaiter(SingleThreadedMutex* m):m(m){}
    private:

        SingleThreadedMutex* m;
    };

    awaiter operator co_await() {
        return awaiter{this};
    }

    SingleThreadedMutex() = default;
    SingleThreadedMutex(const SingleThreadedMutex&) = delete;
    SingleThreadedMutex(SingleThreadedMutex&& o) {
        assert(!o.active);
        waiting = std::move(o.waiting);
        pool = o.pool;
    }

    SingleThreadedMutex& operator=(const SingleThreadedMutex&) = delete;
    SingleThreadedMutex& operator=(SingleThreadedMutex&& o) {
        if (this == &o) return *this;
        assert(!o.active);
        assert(!active);
        waiting = std::move(o.waiting);
        pool = o.pool;
        return *this;
    };
private:
    bool active = false;
    std::deque<std::coroutine_handle<>> waiting;
    CoroutineThreadPool* pool;
    friend class MutexGuard;
};

template<>
struct AwaitTransformPassThrough<SingleThreadedMutex> {
    static constexpr bool pass_through = true;
};

}