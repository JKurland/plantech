#pragma once

#include <type_traits>
#include <iostream>
#include <coroutine>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <chrono>

#include "thread_pool/thread_pool.h"
#include "thread_pool/promise.h"

#include "framework/concepts.h"
#include "framework/handler_set.h"

namespace pt {

namespace context::detail {
    template<size_t I, size_t...Is>
    constexpr size_t get_first(std::index_sequence<I, Is...>) {return I;}


    struct joinable {
        struct promise_type;

        struct final_awaitable {
            bool await_ready() noexcept {return false;}
            std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
                int prev_running_count = (*promise->running_count)--;
                if (prev_running_count == 1) {
                    return *promise->continuation;
                } else {
                    return std::noop_coroutine();
                }
            }
            void await_resume() noexcept {}

            promise_type* promise;
        };

        struct promise_type {
            promise_type(Task<>&&, std::atomic<int>& running_count, std::coroutine_handle<>& continuation):
                running_count(&running_count),
                continuation(&continuation) {

            }

            joinable get_return_object() noexcept {return {this};}

            std::suspend_always initial_suspend() const noexcept {
                return {};
            }
            final_awaitable final_suspend() noexcept {
                return final_awaitable{this};
            }
            void unhandled_exception() {
                try {
                    std::rethrow_exception(std::current_exception());
                } catch (const std::exception& e) {
                    std::cout << "Exception while handling event: " << e.what() << std::endl;
                } catch (...) {
                    std::cout << "Unknown exception while handling event" << std::endl;
                }
            }

            std::atomic<int>* running_count;
            std::coroutine_handle<>* continuation;
            CoroutineThreadPool* pool;
        };
        
        struct awaitable {
            bool await_ready() noexcept {return false;}
            template<typename U>
            std::coroutine_handle<> await_suspend(std::coroutine_handle<U> h) noexcept {
                promise->pool = h.promise().pool;
                promise->pool->push(std::coroutine_handle<promise_type>::from_promise(*promise));
                return h;
            }
            void await_resume() noexcept {
            }

            promise_type* promise;
        };

        awaitable operator co_await() {
            return awaitable{promise};
        }

        ~joinable() {
            std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
        }

        promise_type* promise;
    };

    joinable make_joinable(Task<>&& task, std::atomic<int>& running_count, std::coroutine_handle<>& continuation);

    struct joined {
        struct promise_type;

        struct final_awaitable {
            bool await_ready() noexcept {return false;}
            std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
                void* expected = nullptr;
                if (!promise->continuation.compare_exchange_strong(expected, this)) {
                    return std::coroutine_handle<>::from_address(promise->continuation.load());
                } else {
                    return std::noop_coroutine();
                }
            }
            void await_resume() noexcept {}

            promise_type* promise;
        };

        struct promise_type {
            template<typename...Ts>
            promise_type(CoroutineThreadPool& pool, Ts&&...):pool(&pool) {
            }

            joined get_return_object() {
                return joined{this};
            }

            std::suspend_never initial_suspend() {
                return {};
            }
            final_awaitable final_suspend() noexcept {
                return {this};
            }

            void unhandled_exception() {
                assert(false);
            }

            std::atomic<void*> continuation;
            CoroutineThreadPool* pool;
        };


        bool await_ready() noexcept {
            return promise->continuation.load() != nullptr;
        }

        template<typename U>
        bool await_suspend(std::coroutine_handle<U> handle) noexcept {
            void* expected = nullptr;
            if (promise->continuation.compare_exchange_strong(expected, handle.address())) {
                return true;
            } else {
                return false;
            }
        }

        void await_resume() noexcept {}

        ~joined() {
            if (promise) {
                std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
            }
        }

        joined(promise_type* promise): promise(promise) {}

        joined(const joined&) = delete;
        joined& operator=(const joined&) = delete;

        joined(joined&& o) {
            promise = o.promise;
            o.promise = nullptr;
        }
        joined& operator=(joined&& o) {
            std::swap(this->promise, o.promise);
            return *this;
        }

        promise_type* promise;
    };

    struct wait_for_joined {
        bool await_ready() noexcept {
            return running_count->load() == -num_joined;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<> h) noexcept {
            *continuation = h;
            int prev_running_count = running_count->fetch_add(num_joined);
            if (prev_running_count == -num_joined) {
                return h;
            } else {
                return std::noop_coroutine();
            }
        }

        void await_resume() noexcept {}

        std::atomic<int>* running_count;
        std::coroutine_handle<>* continuation;
        int num_joined;
    };

    template<typename F, Event E, IsContext C, typename...HandlerTs>
    joined join(CoroutineThreadPool& pool, F&& done_cb, C& ctx, E event, HandlerTs&...handlers) {
        std::atomic<int> running_count = 0;
        std::coroutine_handle<> continuation;

        (co_await make_joinable(handlers.handle(ctx, event), running_count, continuation), ..., co_await wait_for_joined{&running_count, &continuation, sizeof...(HandlerTs)});
        done_cb(ctx);
    }

    template<IsContext C, Event E>
    struct EventPred {
        template<typename Handler>
        static constexpr bool value = HandlesEvent<Handler, C, E>;
    };

    template<IsContext C, Request R>
    struct RequestPred {
        template<typename Handler>
        static constexpr bool value = HandlesRequest<Handler, C, R>;
    };

    template<typename T, typename...ArgTs>
    struct CtorArgs {
        CtorArgs(ArgTs&&...args): args(std::forward<ArgTs>(args)...) {}
        using Type = T;

        // we always want a reference
        std::tuple<ArgTs&&...> args;
    };


    template<typename T>
    struct is_ctor_args: std::false_type {};

    template<typename...Ts>
    struct is_ctor_args<CtorArgs<Ts...>>: std::true_type {};

    struct make_context_friend;
}

template<typename T, typename...ArgTs>
context::detail::CtorArgs<T, ArgTs...> ctor_args(ArgTs&&...args) {
    return context::detail::CtorArgs<T, ArgTs...>(std::forward<ArgTs>(args)...);
}

template<typename...HandlerTs>
class Context {
public:
    Context(const Context&) = delete;
    Context(Context&&) = delete;

    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

    template<bool AllowUnhandled=true, Event E>
    void emit(E&& event) {
        run_awaitable_async(thread_pool, emit_await<AllowUnhandled>(std::forward<E>(event)));
    }

    template<bool AllowUnhandled=true, Event E>
    void emit_sync(E&& event) {
        run_awaitable_sync(thread_pool, emit_await<AllowUnhandled>(std::forward<E>(event)));
    }

    template<bool AllowUnhandled=true, Event E>
    auto emit_await(E&& event) {
        assert(!stopped);
        constexpr auto indexes = handler_set.template true_indexes<context::detail::EventPred<Context, E>>();
        static_assert(indexes.size() != 0 || AllowUnhandled, "Nothing to handle event E");
        if constexpr (indexes.size() != 0) {
            start_event();
            return handler_set.call_with(
                indexes,
                [&](auto&...handlers) {
                    return context::detail::join(
                        thread_pool,
                        [](Context& ctx){ctx.end_event();},
                        *this,
                        std::forward<E>(event),
                        handlers...
                    );
                }
            );
        } else {
            return std::suspend_never{};
        }
    }

    template<Request R>
    auto operator()(const R& request) {
        assert(!stopped);
        constexpr auto indexes = handler_set.template true_indexes<context::detail::RequestPred<Context, R>>();
        static_assert(indexes.size() != 0, "Nothing to handle request R");
        static_assert(indexes.size() < 2, "More than one handler for request R");

        if constexpr (indexes.size() == 1) {
            auto& handler = handler_set.template get<context::detail::get_first(indexes)>();
            return handler.handle(*this, request);
        } else {
            // the static asserts have already failed but to stop unhelpful compiler error messages we
            // still return something from this function
            return Task<typename R::ResponseT>{};
        }
    }

    template<Request R>
    auto request_sync(const R& request) {
        assert(!stopped);
        return run_awaitable_sync(thread_pool, (*this)(request));
    }

    template<Event E>
    static constexpr bool can_handle() {
        constexpr auto indexes = decltype(handler_set)::template true_indexes<context::detail::EventPred<Context, E>>();
        return indexes.size() > 0;
    }

    void wait_for_all_events_to_finish() {
        std::unique_lock l(m);
        cv.wait(l, [&]{return events_in_progress == 0;});
    }

    // should only be called by main or tests
    void no_more_messages() {
        stopped = true;
    }

    ~Context() {
        wait_for_all_events_to_finish();
    }
private:
    HandlerSet<HandlerTs...> handler_set;

    std::atomic<size_t> events_in_progress = 0;
    std::mutex m;
    std::condition_variable cv;

    FixedCoroutineThreadPool<1> thread_pool;

    std::atomic<bool> stopped = false;

    template<typename...OtherHandlerTs, typename NewHandlerT>
    Context(Context<OtherHandlerTs...>&& old, NewHandlerT&& new_handler):
        handler_set(std::move(old.handler_set), std::forward<NewHandlerT>(new_handler))
    {}

    Context(): handler_set() {}

    void start_event() {
        events_in_progress++;
    }

    void end_event() {
        size_t prev = events_in_progress--;
        if (prev == 1) {
            cv.notify_all();
        }
    }

    friend context::detail::make_context_friend;

    template<typename...Ts>
    friend class Context;
};


namespace context::detail {
    struct make_context_friend {
        template<template<typename...> typename CtxT, typename...PrevHandlerTs, typename FirstT, typename...ArgTs>
        static auto make_context_impl(CtxT<PrevHandlerTs...>&& prev_ctx, FirstT&& first, ArgTs&&...args) {
            if constexpr (is_ctor_args<std::decay_t<FirstT>>::value) {
                auto new_handler = std::apply(
                    [&](auto&&...ctor_args){return typename FirstT::Type(prev_ctx, std::forward<decltype(ctor_args)>(ctor_args)...);},
                    first.args
                );
                if constexpr (sizeof...(ArgTs) == 0) {
                    return CtxT<PrevHandlerTs..., decltype(new_handler)>(std::move(prev_ctx), std::move(new_handler));
                } else {
                    return make_context_impl(CtxT<PrevHandlerTs..., decltype(new_handler)>(std::move(prev_ctx), std::move(new_handler)), std::forward<ArgTs>(args)...);
                }
            } else {
                if constexpr (sizeof...(ArgTs) == 0) {
                    return CtxT<PrevHandlerTs..., std::decay_t<FirstT>>(std::move(prev_ctx), std::forward<FirstT>(first));
                } else {
                    return make_context_impl(CtxT<PrevHandlerTs..., std::decay_t<FirstT>>(std::move(prev_ctx), std::forward<FirstT>(first)), std::forward<ArgTs>(args)...);
                }
            }
        }

        template<template<typename...> typename CtxT, typename...ArgTs>
        static auto make_context_impl_outer(ArgTs&&...args) {
            return make_context_impl(CtxT<>(), std::forward<ArgTs>(args)...);
        }
    };
} // context::detail


template<typename...ArgTs>
auto make_context(ArgTs&&...args) {
    return context::detail::make_context_friend::make_context_impl_outer<Context>(std::forward<ArgTs>(args)...);
}


#define EVENT(event_type) \
template<IsContext C> \
Task<> handle(C& ctx, const event_type& event)

#define TEMPLATE_EVENT(event_type, ...) \
template<IsContext C, __VA_ARGS__> \
Task<> handle(C& ctx, const event_type& event)

#define REQUEST(request_type) \
template<IsContext C> \
Task<request_type::ResponseT> handle(C& ctx, const request_type& request)

#define TEMPLATE_REQUEST(request_type, ...) \
template<IsContext C, __VA_ARGS__> \
Task<typename request_type::ResponseT> handle(C& ctx, const request_type& request)

}