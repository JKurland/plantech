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
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                void* expected = nullptr;
                if (promise->continuation.compare_exchange_strong(expected, handle.address())) {
                    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                    return true;
                } else {
                    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                    return false;
                }
            }

            InnerT await_resume() {
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                return false;
            }
            void await_suspend(std::coroutine_handle<>) noexcept {
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                if (promise->continuation.load() != promise) {
                    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                return {};
            }
            final_awaitable final_suspend() noexcept {
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                return {this};
            }

            void return_value(InnerT&& v) {
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
                return_value_.emplace(std::move(v));
                void* expected = nullptr;
                continuation.compare_exchange_strong(expected, this);
            };

            void return_value(const InnerT& v) {
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return false;
        }

        void await_suspend(std::coroutine_handle<>) noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            {
                std::lock_guard l(promise->m);
                promise->finished = true;
            }
            promise->cv.notify_all();
        }

        constexpr void await_resume() const noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        }

        promise_type* promise;
    };

    struct initial_awaitable {
        constexpr bool await_ready() const noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return false;
        }

        void await_suspend(std::coroutine_handle<> handle) {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            promise->pool->push(handle);
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        }

        constexpr void await_resume() const noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        }

        promise_type* promise;
    };

    struct promise_type {

        template<typename ThisT>
        promise_type(ThisT this_, CoroutineThreadPool& pool): pool(&pool) {}


        constexpr run_sync get_return_object() noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return {this};
        }

        // puts this coroutine on the thread pool
        constexpr initial_awaitable initial_suspend() noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return {this};
        }

        // notifies that this coroutine has finished
        constexpr final_awaitable final_suspend() noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return {this};
        }

        void return_value(T&& t) {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return_value_.emplace(std::move(t));
        }

        void return_value(const T& t) {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return_value_.emplace(t);
        }

        void unhandled_exception() {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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
        std::cout << __LINE__ << ", " << promise->return_value_.has_value() << std::endl;
        return std::move(promise->return_value_.value());
    }

    ~run_sync() {
        std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
    }

    promise_type* promise;
};


template<typename T>
class Task {
public:
    struct promise_type;

    struct final_awaitable {
        constexpr bool await_ready() const noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return false;
        }

        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return promise->continuation;
        }

        constexpr void await_resume() const noexcept {}
        promise_type* promise;
    };

    struct promise_type {
        constexpr Task get_return_object() noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return Task{this};
        }

        constexpr std::suspend_always initial_suspend() const noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return {};
        }
        constexpr final_awaitable final_suspend() noexcept {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return {this};
        }

        template<std::convertible_to<T> V>
        void return_value(V&& v) {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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
                std::cout << __FILE__ << ": " << __LINE__ << std::endl;
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
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            promise->pool = handle.promise().pool;
            promise->continuation = handle;
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            return std::coroutine_handle<promise_type>::from_promise(*promise);
        }

        T await_resume() {
            std::cout << __FILE__ << ": " << __LINE__ << std::endl;
            if constexpr (MoveOnResume) {
                return std::move(promise->return_value_.value());
            } else {
                return promise->return_value_.value();
            }
        }

        promise_type* promise;
    };

    constexpr auto operator co_await() & noexcept {
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        return awaitable<false>{promise};
    }

    constexpr auto operator co_await() && noexcept {
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        return awaitable<true>{promise};
    }

    ~Task() {
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    }

    promise_type* promise;
};


template<typename F, typename...ArgTs>
auto run(CoroutineThreadPool& pool, F&& f, ArgTs&&...args) {
    using invoke_result = std::invoke_result_t<F&&, ArgTs&&...>;
    using T = promise::detail::ResultT<invoke_result>;

    run_sync coro = [&](CoroutineThreadPool& pool) -> run_sync<T> {
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        auto x = std::invoke(std::forward<F>(f), std::forward<ArgTs>(args)...);
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        auto ret = co_await std::move(x);
        std::cout << __FILE__ << ": " << __LINE__ << std::endl;
        co_return std::move(ret);
    }(pool);
    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    coro.wait();
    std::cout << __FILE__ << ": " << __LINE__ << std::endl;
    return std::move(coro).get();
}


}