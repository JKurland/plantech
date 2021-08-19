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

    template<Event E, IsContext C, typename...HandlerTs>
    joined join(CoroutineThreadPool& pool, C& ctx, E event, HandlerTs&...handlers) {
        std::atomic<int> running_count = 0;
        std::coroutine_handle<> continuation;

        (co_await make_joinable(handlers.handle(ctx, event), running_count, continuation), ..., co_await wait_for_joined{&running_count, &continuation, sizeof...(HandlerTs)});
        ctx.end_event();
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
}

template<typename...HandlerTs>
class Context {
public:
    template<typename...ArgTs>
    Context(ArgTs&&...args): handler_set(HandlerSet(std::forward<ArgTs>(args)...)) {}

    Context(const Context&) = delete;
    Context(Context&&) = delete;

    Context& operator=(const Context&) = delete;
    Context& operator=(Context&&) = delete;

    template<bool AllowUnhandled=true, Event E>
    void emit(E&& event) {
        constexpr auto indexes = handler_set.template true_indexes<context::detail::EventPred<Context, E>>();
        static_assert(indexes.size() != 0 || AllowUnhandled, "Nothing to handle event E");
        if constexpr (indexes.size() != 0) {
            start_event();
            auto joined = handler_set.call_with(
                indexes,
                [&](auto&...handlers) {
                    return context::detail::join(thread_pool, *this, std::forward<E>(event), handlers...);
                }
            );
            run_awaitable_async(thread_pool, std::move(joined));
        }   
    }

    template<bool AllowUnhandled=true, Event E>
    void emit_sync(E&& event) {
        constexpr auto indexes = handler_set.template true_indexes<context::detail::EventPred<Context, E>>();
        static_assert(indexes.size() != 0 || AllowUnhandled, "Nothing to handle event E");
        if constexpr (indexes.size() != 0) {
            start_event();
            auto joined = handler_set.call_with(
                indexes,
                [&](auto&...handlers) {
                    return context::detail::join(thread_pool, *this, std::forward<E>(event), handlers...);
                }
            );
            run_awaitable_sync(thread_pool, std::move(joined));
        }   
    }

    template<bool AllowUnhandled=true, Event E>
    auto emit_await(E&& event) {
        constexpr auto indexes = handler_set.template true_indexes<context::detail::EventPred<Context, E>>();
        static_assert(indexes.size() != 0 || AllowUnhandled, "Nothing to handle event E");
        if constexpr (indexes.size() != 0) {
            start_event();
            return handler_set.call_with(
                indexes,
                [&](auto&...handlers) {
                    return context::detail::join(thread_pool, *this, std::forward<E>(event), handlers...);
                }
            );
        } else {
            return std::suspend_never{};
        }
    }

    template<Request R>
    auto operator()(const R& request) {
        constexpr auto indexes = handler_set.template true_indexes<context::detail::RequestPred<Context, R>>();
        static_assert(indexes.size() != 0, "Nothing to handle request R");
        static_assert(indexes.size() == 1, "More than one handler for request R");

        if constexpr (indexes.size() == 1) {
            auto& handler = handler_set.template get<context::detail::get_first(indexes)>();
            return handler.handle(*this, request);
        }
    }

    void start_event() {
        events_in_progress++;
    }

    void end_event() {
        size_t prev = events_in_progress--;
        if (prev == 1) {
            cv.notify_all();
        }
    }

    void wait_for_all_events_to_finish() {
        std::unique_lock l(m);
        cv.wait(l, [&]{return events_in_progress == 0;});
    }

    template<Request R>
    auto request_sync(const R& request) {
        return run_awaitable_sync(thread_pool, (*this)(request));
    }

    template<Event E>
    static constexpr bool can_handle() {
        constexpr auto indexes = decltype(handler_set)::template true_indexes<context::detail::EventPred<Context, E>>();
        return indexes.size() > 0;
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
};

template<typename...ArgTs>
Context(ArgTs...) -> Context<std::decay_t<ArgTs>...>;


#define EVENT(event_type) \
template<IsContext C> \
Task<> handle(C& ctx, const event_type& event)


#define REQUEST(request_type) \
template<IsContext C> \
Task<request_type::ResponseT> handle(C& ctx, const request_type& request)

}