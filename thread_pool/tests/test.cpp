#include <gtest/gtest.h>
#include <memory>
#include <iostream>

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


TEST_F(SingleThreadedThreadPoolTest, should_run_something) {
    auto ret = run(pool, []() -> Task<int> {
        co_return 1;
    });
    ASSERT_EQ(ret, 1);
}

TEST_F(SingleThreadedThreadPoolTest, should_run_something_nested) {
    auto nested = []() -> Task<int> {
        co_return 2;
    };

    auto ret = run(pool, [&]() -> Task<int> {
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

    run(pool, coro);
    new_thread.join();
    ASSERT_EQ(initial_thread, final_thread);
}


TEST_F(SingleThreadedThreadPoolTest, should_work_with_void) {
    bool ran = false;
    run(pool,  [&]() -> Task<> {
        ran = true;
        co_return;
    });

    ASSERT_TRUE(ran);
}