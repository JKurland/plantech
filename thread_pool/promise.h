#pragma once
#include "thread_pool/thread_pool.h"
#include <type_traits>
#include <concepts>
#include <optional>
#include <utility>
#include <iostream>

namespace pt {

namespace promise::detail {

    template<typename AwaitableT>
    auto result_t_helper() {
        if constexpr (requires(AwaitableT a) {a.operator co_await();}) {
            using T = decltype((std::declval<AwaitableT>().operator co_await()).await_resume())*;
            return T{};
        } else {
            using T = decltype(std::declval<AwaitableT>().await_resume())*;
            return T{};
        }
    }

    template<typename AwaitableT>
    using ResultT = std::remove_pointer_t<decltype(result_t_helper<AwaitableT>())>;


    template<typename InnerT>
    struct wrapper_task {
        struct promise_type;

        template<bool MoveOnResume>
        struct awaitable {
            bool await_ready() {return promise->continuation.load() != nullptr;}
            bool await_suspend(std::coroutine_handle<> handle) {
                void* expected = nullptr;
                if (promise->continuation.compare_exchange_strong(expected, handle.address())) {
                    return true;
                } else {
                    return false;
                }
            }

            InnerT await_resume() {
                if constexpr (MoveOnResume) {
                    return std::move(promise->return_value_.value());
                } else {
                    return promise->return_value_.value();
                }
            }
            promise_type* promise;
        };

        struct final_awaitable {
            bool await_ready() const noexcept {
                return false;
            }
            void await_suspend(std::coroutine_handle<>) noexcept {
                if (promise->continuation.load() != promise) {
                    promise->pool->push(std::coroutine_handle<>::from_address(promise->continuation.load()));
                }
            }
            void await_resume() const noexcept {}
            promise_type* promise;
        };

        struct promise_type {

            template<typename ThisT>
            promise_type(ThisT this_, CoroutineThreadPool& pool): pool(&pool) {}

            wrapper_task get_return_object() {
                return {this};
            }

            std::suspend_never initial_suspend() noexcept {
                return {};
            }
            final_awaitable final_suspend() noexcept {
                return {this};
            }

            void return_value(InnerT&& v) {
                return_value_.emplace(std::move(v));
                void* expected = nullptr;
                continuation.compare_exchange_strong(expected, this);
            };

            void return_value(const InnerT& v) {
                return_value_.emplace(v);
                void* expected = nullptr;
                continuation.compare_exchange_strong(expected, this);
            };

            void unhandled_exception() {}

            std::optional<InnerT> return_value_;
            CoroutineThreadPool* pool;
            std::atomic<void*> continuation = nullptr;
        };

        ~wrapper_task() {
            std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
        }

        awaitable<false> operator co_await() & {
            return {promise};
        }

        awaitable<true> operator co_await() && {
            return {promise};
        }

        promise_type* promise;
    };


}

template<typename T>
struct run_sync {
    struct promise_type;

    struct final_awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<>) noexcept {
            {
                std::lock_guard l(promise->m);
                promise->finished = true;
            }
            promise->cv.notify_all();
        }

        constexpr void await_resume() const noexcept {
        }

        promise_type* promise;
    };

    struct initial_awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            promise->pool->push(handle);
        }

        constexpr void await_resume() const noexcept {
        }

        promise_type* promise;
    };

    struct promise_type {

        template<typename ThisT>
        promise_type(ThisT this_, CoroutineThreadPool& pool): pool(&pool) {}


        constexpr run_sync get_return_object() noexcept {
            return {this};
        }

        // puts this coroutine on the thread pool
        constexpr initial_awaitable initial_suspend() noexcept {
            return {this};
        }

        // notifies that this coroutine has finished
        constexpr final_awaitable final_suspend() noexcept {
            return {this};
        }

        void return_value(T&& t) {
            return_value_.emplace(std::move(t));
        }

        void return_value(const T& t) {
            return_value_.emplace(t);
        }

        void unhandled_exception() {
        }

        std::condition_variable cv;
        bool finished = false;
        std::mutex m;
        CoroutineThreadPool* pool;
        std::optional<T> return_value_;
    };

    void wait() {
        std::unique_lock l(promise->m);
        promise->cv.wait(l, [&]{return promise->finished;});
    }

    constexpr T&& get() && {
        return std::move(promise->return_value_.value());
    }

    ~run_sync() {
        std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
    }

    promise_type* promise;
};


template<>
struct run_sync<void> {
    struct promise_type;

    struct final_awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<>) noexcept {
            {
                std::lock_guard l(promise->m);
                promise->finished = true;
            }
            promise->cv.notify_all();
        }

        constexpr void await_resume() const noexcept {
        }

        promise_type* promise;
    };

    struct initial_awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            promise->pool->push(handle);
        }

        constexpr void await_resume() const noexcept {
        }

        promise_type* promise;
    };

    struct promise_type {

        template<typename ThisT>
        promise_type(ThisT this_, CoroutineThreadPool& pool): pool(&pool) {}


        run_sync get_return_object() noexcept {
            return {this};
        }

        // puts this coroutine on the thread pool
        constexpr initial_awaitable initial_suspend() noexcept {
            return {this};
        }

        // notifies that this coroutine has finished
        constexpr final_awaitable final_suspend() noexcept {
            return {this};
        }

        void return_void() {}

        void unhandled_exception() {
        }

        std::condition_variable cv;
        bool finished = false;
        std::mutex m;
        CoroutineThreadPool* pool;
    };

    void wait() {
        std::unique_lock l(promise->m);
        promise->cv.wait(l, [&]{return promise->finished;});
    }

    ~run_sync() {
        std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
    }

    promise_type* promise;
};



template<typename T=void>
class Task {
public:
    struct promise_type;

    struct final_awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
            return promise->continuation;
        }

        constexpr void await_resume() const noexcept {}
        promise_type* promise;
    };

    struct promise_type {
        constexpr Task get_return_object() noexcept {
            return Task{this};
        }

        constexpr std::suspend_always initial_suspend() const noexcept {
            return {};
        }
        constexpr final_awaitable final_suspend() noexcept {
            return {this};
        }

        template<std::convertible_to<T> V>
        void return_value(V&& v) {
            return_value_.emplace(std::forward<V>(v));
        };

        void unhandled_exception() {}

        template<typename U>
        Task<U>&& await_transform(Task<U>&& awaitable) {
            return std::move(awaitable);
        }

        template<typename U>
        Task<U>& await_transform(Task<U>& awaitable) {
            return awaitable;
        }

        template<typename U>
        const Task<U>&& await_transform(const Task<U>&& awaitable) {
            return std::move(awaitable);
        }

        template<typename U>
        const Task<U>& await_transform(const Task<U>& awaitable) {
            return awaitable;
        }

        template<typename AwaitableT>
        auto await_transform(AwaitableT&& awaitable) {
            using InnerT = promise::detail::ResultT<std::decay_t<AwaitableT>>;


            return [a = std::forward<AwaitableT>(awaitable)](CoroutineThreadPool& pool) mutable -> promise::detail::wrapper_task<InnerT> {
                co_return co_await std::move(a);
            }(*pool);

        }

        std::coroutine_handle<> continuation;
        CoroutineThreadPool* pool;
        std::optional<T> return_value_;
    };

    template<bool MoveOnResume>
    struct awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        template<typename U>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<U> handle) noexcept {
            promise->pool = handle.promise().pool;
            promise->continuation = handle;
            return std::coroutine_handle<promise_type>::from_promise(*promise);
        }

        T await_resume() {
            if constexpr (MoveOnResume) {
                return std::move(promise->return_value_.value());
            } else {
                return promise->return_value_.value();
            }
        }

        promise_type* promise;
    };

    constexpr auto operator co_await() & noexcept {
        return awaitable<false>{promise};
    }

    constexpr auto operator co_await() && noexcept {
        return awaitable<true>{promise};
    }

    ~Task() {
        std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
    }

    promise_type* promise;
};


template<>
class Task<void> {
public:
    struct promise_type;

    struct final_awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
            return promise->continuation;
        }

        constexpr void await_resume() const noexcept {}
        promise_type* promise;
    };

    struct promise_type {
        Task get_return_object() noexcept {
            return Task{this};
        }

        constexpr std::suspend_always initial_suspend() const noexcept {
            return {};
        }
        constexpr final_awaitable final_suspend() noexcept {
            return {this};
        }

        void return_void() {
        };

        void unhandled_exception() {}

        template<typename U>
        Task<U>&& await_transform(Task<U>&& awaitable) {
            return std::move(awaitable);
        }

        template<typename U>
        Task<U>& await_transform(Task<U>& awaitable) {
            return awaitable;
        }

        template<typename U>
        const Task<U>&& await_transform(const Task<U>&& awaitable) {
            return std::move(awaitable);
        }

        template<typename U>
        const Task<U>& await_transform(const Task<U>& awaitable) {
            return awaitable;
        }

        template<typename AwaitableT>
        auto await_transform(AwaitableT&& awaitable) {
            using InnerT = promise::detail::ResultT<std::decay_t<AwaitableT>>;


            return [a = std::forward<AwaitableT>(awaitable)](CoroutineThreadPool& pool) mutable -> promise::detail::wrapper_task<InnerT> {
                co_return co_await std::move(a);
            }(*pool);

        }

        std::coroutine_handle<> continuation;
        CoroutineThreadPool* pool;
    };

    struct awaitable {
        constexpr bool await_ready() const noexcept {
            return false;
        }

        template<typename U>
        std::coroutine_handle<> await_suspend(std::coroutine_handle<U> handle) noexcept {
            promise->pool = handle.promise().pool;
            promise->continuation = handle;
            return std::coroutine_handle<promise_type>::from_promise(*promise);
        }

        void await_resume() {}

        promise_type* promise;
    };

    constexpr auto operator co_await() noexcept {
        return awaitable{promise};
    }

    ~Task() {
        std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
    }

    promise_type* promise;
};



template<typename F, typename...ArgTs>
auto run(CoroutineThreadPool& pool, F&& f, ArgTs&&...args) {
    using invoke_result = std::invoke_result_t<F&&, ArgTs&&...>;
    using T = promise::detail::ResultT<invoke_result>;

    run_sync coro = [&](CoroutineThreadPool& pool) -> run_sync<T> {
        auto x = std::invoke(std::forward<F>(f), std::forward<ArgTs>(args)...);
        if constexpr (std::is_void_v<T>) {
            co_await std::move(x);
        } else {
            co_return co_await std::move(x);
        }
    }(pool);
    coro.wait();
    if constexpr (std::is_void_v<T>) {
        return;
    } else {
        return std::move(coro).get();
    }
}


}