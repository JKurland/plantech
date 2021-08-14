#include <gtest/gtest.h>

#include <functional>

#include "framework/context.h"
#include "framework/concepts.h"
#include "thread_pool/promise.h"

using namespace pt;

struct TestEvent {};
struct TestEvent2 {};

template<typename F>
struct TestHandler {
    template<IsContext C, Event E, typename = std::enable_if_t<std::is_invocable_v<F, C&, const E&>>>
    Task<> handle(C& ctx, const E& e) {
        f(ctx, e);
        co_return;
    }

    F f;
};


TEST(TestContext, should_call_handler_when_event_emitted) {
    bool called = false;
    auto context = Context(
        TestHandler{
            [&](auto&, TestEvent){called = true;}
        }
    );
    context.emit(TestEvent{});
    context.wait_for_all_events_to_finish();

    ASSERT_TRUE(called);
}

TEST(TestContext, should_call_handler_when_event_emitted_nested) {
    bool called = false;
    auto context = Context(
        TestHandler{
            [](auto& ctx, TestEvent){ctx.emit(TestEvent2{});}
        },
        TestHandler{
            [&](auto& ctx, TestEvent2){called = true;}
        }
    );

    context.emit(TestEvent{});
    context.wait_for_all_events_to_finish();

    ASSERT_TRUE(called);
}