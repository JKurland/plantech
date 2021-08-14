#include <gtest/gtest.h>
#include <memory>
#include <future>

#include "thread_pool/thread_pool.h"
#include "thread_pool/promise.h"

using namespace pt;

class SingleThreadedThreadPoolTest: public ::testing::Test {
protected:
    ~SingleThreadedThreadPoolTest() {pool.stop_and_join();}
    FixedCoroutineThreadPool<1> pool;
};


struct swap_threads {
    bool await_ready() {return false;}
    void await_suspend(std::coroutine_handle<> h) noexcept {
        *new_thread = std::thread([=]{
            h.resume();
        });
    }
    int await_resume() {return 2;}
    std::thread* new_thread;
};

struct swap_threads_throw {
    bool await_ready() {return false;}
    void await_suspend(std::coroutine_handle<> h) noexcept {
        *new_thread = std::thread([=]{
            h.resume();
        });
    }
    int await_resume() {throw 2;}
    std::thread* new_thread;
};


TEST_F(SingleThreadedThreadPoolTest, should_run_something) {
    auto ret = run_sync(pool, []() -> Task<int> {
        co_return 1;
    });
    ASSERT_EQ(ret, 1);
}

TEST_F(SingleThreadedThreadPoolTest, should_run_something_nested) {
    auto nested = []() -> Task<int> {
        co_return 2;
    };

    auto ret = run_sync(pool, [&]() -> Task<int> {
        int ans = co_await nested();
        co_return ans;
    });
    ASSERT_EQ(ret, 2);
}

TEST_F(SingleThreadedThreadPoolTest, should_always_run_on_the_thread_pool_thread) {
    std::thread new_thread;
    std::thread::id initial_thread;
    std::thread::id final_thread;

    auto coro = [&]() -> Task<int> {
        initial_thread = std::this_thread::get_id();
        co_await swap_threads{&new_thread};
        final_thread = std::this_thread::get_id();
        co_return 1;
    };

    run_sync(pool, coro);
    new_thread.join();
    ASSERT_EQ(initial_thread, final_thread);
}

TEST_F(SingleThreadedThreadPoolTest, should_always_run_on_the_thread_pool_thread_nested) {
    std::thread new_thread;
    std::thread::id thread1;
    std::thread::id thread2;
    std::thread::id thread3;
    std::thread::id thread4;

    auto nested = [&]() -> Task<> {
        thread2 = std::this_thread::get_id();
        co_await swap_threads{&new_thread};
        thread3 = std::this_thread::get_id();
    };

    auto coro = [&]() -> Task<int> {
        thread1 = std::this_thread::get_id();
        co_await nested();
        thread4 = std::this_thread::get_id();
        co_return 1;
    };

    run_sync(pool, coro);
    new_thread.join();
    ASSERT_EQ(thread1, thread2);
    ASSERT_EQ(thread1, thread3);
    ASSERT_EQ(thread1, thread4);
}

TEST_F(SingleThreadedThreadPoolTest, should_work_with_void_return) {
    bool ran = false;
    run_sync(pool,  [&]() -> Task<> {
        ran = true;
        co_return;
    });

    ASSERT_TRUE(ran);
}

TEST_F(SingleThreadedThreadPoolTest, should_propagate_exceptions_with_void_return) {
    auto coro = []() -> Task<> {
        throw 2;
        co_return;
    };

    ASSERT_THROW(run_sync(pool, coro), int);
}


TEST_F(SingleThreadedThreadPoolTest, should_propagate_exceptions_with_non_void_return) {
    auto coro = []() -> Task<float> {
        throw 2;
        co_return 2.3;
    };

    ASSERT_THROW(run_sync(pool, coro), int);
}


TEST_F(SingleThreadedThreadPoolTest, should_propagate_exceptions_multiple_levels) {
    auto nested = []() -> Task<> {
        throw 2;
        co_return;
    };

    auto coro = [&]() -> Task<int> {
        co_await nested();
        co_return 1;
    };

    ASSERT_THROW(run_sync(pool, coro), int);
}

TEST_F(SingleThreadedThreadPoolTest, should_propagate_exceptions_multiple_levels_multiple_co_awaits) {
    auto nested = [](bool throw_) -> Task<> {
        if (throw_) {
            throw 2;
        }
        co_return;
    };

    bool past_non_throwing_co_await = false;
    auto coro = [&]() -> Task<int> {
        co_await nested(false);
        past_non_throwing_co_await = true;
        co_await nested(true);
        co_return 1;
    };

    ASSERT_THROW(run_sync(pool, coro), int);
    ASSERT_EQ(past_non_throwing_co_await, true);
}


TEST_F(SingleThreadedThreadPoolTest, should_propagate_exceptions_from_wrapped_tasks) {
    std::thread t;
    std::thread::id start;
    std::thread::id after;

    auto coro = [&]() -> Task<> {
        start = std::this_thread::get_id();
        try {
            co_await swap_threads_throw{&t};
        } catch (...) {
            after = std::this_thread::get_id();
            throw;
        }
    };

    ASSERT_THROW(run_sync(pool, coro), int);
    ASSERT_EQ(start, after);
    t.join();
}


TEST_F(SingleThreadedThreadPoolTest, run_async) {
    std::promise<int> promise;
    std::future<int> future = promise.get_future();
    run_async(pool, [&]() -> Task<int> {
        promise.set_value(3);
        co_return 1;
    });

    
    ASSERT_EQ(future.get(), 3);
}

