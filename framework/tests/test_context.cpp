#include <gtest/gtest.h>

#include <functional>
#include <future>
#include <iostream>

#include "framework/context.h"
#include "framework/concepts.h"
#include "thread_pool/promise.h"

using namespace pt;

struct ConfirmDelivery {
    bool* delivered;
    int depth;
};

struct WaitFor {
    std::future<void> future;
};

struct TestHandler {
    EVENT(const ConfirmDelivery& event) {
        if (event.depth == 1) {
            *event.delivered = true;
        } else {
            co_await ctx.emit_await(ConfirmDelivery{event.delivered, event.depth-1});
        }
        co_return;
    }


    EVENT(const WaitFor& event) {
        struct awaitable {
            struct awaiter {
                bool await_ready() {return false;}
                void await_suspend(std::coroutine_handle<> handle) noexcept {
                    a->t = std::thread([this, handle]{
                        a->f->wait();
                        handle.resume();
                    });
                }
                void await_resume() {}

                awaitable* a;
            };

            awaiter operator co_await() {
                return awaiter{this};
            }

            ~awaitable() {
                if (t.joinable()) {
                    t.join();
                }
            }

            const std::future<void>* f;
            std::thread t;
        };

        co_await awaitable{&event.future};
    }
};

class TestContext: public ::testing::Test {
protected:
    TestContext(): ctx(TestHandler{}) {}

    ~TestContext() {
        for (auto& promise: promises) {
            promise.set_value();
        }
    }

    void add_waiting_handler() {
        promises.emplace_back();
        ctx.emit(WaitFor{promises.back().get_future()});
    }

    void assert_message_can_be_delivered(int depth=1) {
        bool delivered = false;
        ctx.emit_sync(ConfirmDelivery{&delivered, depth});
        ASSERT_TRUE(delivered);
    }

private:
    std::vector<std::promise<void>> promises;
    Context<TestHandler> ctx;
};


TEST_F(TestContext, should_call_handler_when_event_emitted) {
    assert_message_can_be_delivered();
}

TEST_F(TestContext, should_call_handler_when_event_emitted_nested) {
    assert_message_can_be_delivered(2);
}

TEST_F(TestContext, should_allow_messages_while_handler_is_waiting) {
    add_waiting_handler();
    assert_message_can_be_delivered();
}