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

struct AddOne {
    using ResponseT = int;
    int x;
    int depth;
};

struct AsyncSpinUpWaitFor {
    WaitFor* wait_for;
};

struct TestHandler {
    EVENT(ConfirmDelivery) {
        if (event.depth == 1) {
            *event.delivered = true;
        } else {
            co_await ctx.emit_await(ConfirmDelivery{event.delivered, event.depth-1});
        }
        co_return;
    }


    EVENT(WaitFor) {
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

            awaitable(const std::future<void>& f): f(&f) {}
            awaitable(const awaitable&) = delete;

            ~awaitable() {
                if (t.joinable()) {
                    t.join();
                }
            }

            const std::future<void>* f;
            std::thread t;
        };

        co_await awaitable{event.future};
    }

    REQUEST(AddOne) {
        if (request.depth == 1) {
            co_return request.x + 1;
        } else {
            co_return co_await ctx(AddOne{request.x, request.depth - 1});
        }
    }

    EVENT(AsyncSpinUpWaitFor) {
        ctx.emit(std::move(*event.wait_for));
        co_return;
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

    void assert_request_is_answered(int depth=1) {
        int ans = ctx.request_sync(AddOne{
            .x = 3,
            .depth = depth,
        });

        ASSERT_EQ(ans, 4);
    }

    void assert_emit_sync_doesnt_wait_for_unawaited_events() {
        std::promise<void> p;
        auto wait_for = WaitFor{p.get_future()};
        ctx.emit_sync(AsyncSpinUpWaitFor{
            .wait_for = &wait_for,
        });

        // If emit_sync were waiting for the WaitFor that AsyncSpinUpWaitFor emits but doesn't
        // await then we'll never get here.
        p.set_value();
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

TEST_F(TestContext, should_allow_messages_while_many_handlers_are_waiting) {
    for (size_t i = 0; i < 10000; i++) {
        add_waiting_handler();
    }
    assert_message_can_be_delivered();
}

TEST_F(TestContext, should_get_answer_for_request) {
    assert_request_is_answered();
}

TEST_F(TestContext, should_get_answer_for_request_nested) {
    assert_request_is_answered(2);
}

TEST_F(TestContext, should_get_answer_for_request_nested_deep) {
    assert_request_is_answered(10000);
}

TEST_F(TestContext, should_get_answer_for_request_while_handler_is_waiting) {
    add_waiting_handler();
    assert_request_is_answered();
}

TEST_F(TestContext, should_get_answer_for_request_while_handler_is_waiting_nested) {
    add_waiting_handler();
    assert_request_is_answered(2);
}

TEST_F(TestContext, emit_sync_shouldnt_wait_for_unawaited_events) {
    assert_emit_sync_doesnt_wait_for_unawaited_events();
}
