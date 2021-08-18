#pragma once
#include "thread_pool/thread_pool.h"
#include <type_traits>
#include <concepts>
#include <optional>
#include <utility>
#include <cassert>
#include <exception>
#include <variant>
#include <coroutine>
#include <functional>

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
                switch (promise->return_value_.index()) {
                    case 1: {
                        if constexpr (MoveOnResume) {
                            return std::move(std::get<1>(promise->return_value_));
                        } else {
                            return std::get<1>(promise->return_value_);
                        }
                    }
                    case 2: {
                        std::rethrow_exception(std::get<2>(promise->return_value_));
                    }
                }
                assert(false);
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

            template<std::convertible_to<InnerT> V>
            void return_value(V&& v) {
                return_value_.template emplace<1>(std::forward<V>(v));
                void* expected = nullptr;
                continuation.compare_exchange_strong(expected, this);
            };

            void unhandled_exception() {
                return_value_.template emplace<2>(std::current_exception());
                void* expected = nullptr;
                continuation.compare_exchange_strong(expected, this);
            }

            std::variant<std::monostate, InnerT, std::exception_ptr> return_value_;
            CoroutineThreadPool* pool;
            std::atomic<void*> continuation = nullptr;
        };

        wrapper_task(promise_type* promise): promise(promise) {}
        wrapper_task(const wrapper_task&) = delete;
        wrapper_task(wrapper_task&& o) {
            promise = o.promise;
            o.promise = nullptr;
        }

        wrapper_task& operator=(const wrapper_task&) = delete;
        wrapper_task& operator=(wrapper_task&& o) {
            std::swap(promise, o.promise);
            return *this;
        }

        ~wrapper_task() {
            if (promise) {
                std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
            }
        }

        awaitable<false> operator co_await() & {
            return {promise};
        }

        awaitable<true> operator co_await() && {
            return {promise};
        }

        promise_type* promise;
    };


    template<>
    struct wrapper_task<void> {
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

            void await_resume() {
                if (promise->exception) {
                    std::rethrow_exception(promise->exception);
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

            void return_void() {
                void* expected = nullptr;
                continuation.compare_exchange_strong(expected, this);
            }

            void unhandled_exception() {
                exception = std::current_exception();
                void* expected = nullptr;
                continuation.compare_exchange_strong(expected, this);
            }

            std::exception_ptr exception = nullptr;
            CoroutineThreadPool* pool;
            std::atomic<void*> continuation = nullptr;
        };

        wrapper_task(promise_type* promise): promise(promise) {}
        wrapper_task(const wrapper_task&) = delete;
        wrapper_task(wrapper_task&& o) {
            promise = o.promise;
            o.promise = nullptr;
        }

        wrapper_task& operator=(const wrapper_task&) = delete;
        wrapper_task& operator=(wrapper_task&& o) {
            std::swap(promise, o.promise);
            return *this;
        }

        ~wrapper_task() {
            if (promise) {
                std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
            }
        }

        awaitable<false> operator co_await() & {
            return {promise};
        }

        awaitable<true> operator co_await() && {
            return {promise};
        }

        promise_type* promise;
    };

    template<typename T, bool OwnHandle>
    struct run_sync {
        struct promise_type;

        struct final_awaitable {
            constexpr bool await_ready() const noexcept {
                return false;
            }

            auto await_suspend(std::coroutine_handle<>) noexcept {
                {
                    std::lock_guard l(promise->m);
                    promise->finished = true;
                }
                promise->cv.notify_all();
                if constexpr (!OwnHandle) {
                    return false;
                }
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

            template<typename ThisT, typename AwaitableT>
            promise_type(ThisT this_, CoroutineThreadPool& pool, AwaitableT&): pool(&pool) {}

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

            template<std::convertible_to<T> V>
            void return_value(V&& v) {
                return_value_.template emplace<1>(std::forward<V>(v));
            }

            void unhandled_exception() {
                return_value_.template emplace<2>(std::current_exception());
            }

            std::condition_variable cv;
            bool finished = false;
            std::mutex m;
            CoroutineThreadPool* pool;
            std::variant<std::monostate, T, std::exception_ptr> return_value_;
        };

        void wait() {
            std::unique_lock l(promise->m);
            promise->cv.wait(l, [&]{return promise->finished;});
        }

        constexpr T&& get() && {
            switch (promise->return_value_.index()) {
                case 1: {
                    return std::move(std::get<1>(promise->return_value_));
                }
                case 2: {
                    std::rethrow_exception(std::get<2>(promise->return_value_));
                }
            }
            assert(false);
        }

        run_sync(promise_type* promise): promise(promise) {}

        run_sync(const run_sync& o) = delete;
        run_sync& operator=(const run_sync& o) = delete;

        run_sync(run_sync&& o) {
            promise = o.promise;
            if constexpr (OwnHandle) {
                o.promise = nullptr;
            }
        }

        run_sync& operator=(run_sync&& o) {
            std::swap(o.promise, promise);
            return *this;
        }


        ~run_sync() {
            if constexpr (OwnHandle) {
                if (promise) {
                    std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
                }
            }
        }

        promise_type* promise;
    };


    template<bool OwnHandle>
    struct run_sync<void, OwnHandle> {
        struct promise_type;

        struct final_awaitable {
            constexpr bool await_ready() const noexcept {
                return false;
            }

            auto await_suspend(std::coroutine_handle<>) noexcept {
                {
                    std::lock_guard l(promise->m);
                    promise->finished = true;
                }
                promise->cv.notify_all();
                if constexpr (!OwnHandle) {
                    return false;
                }
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

            void await_resume() const noexcept {
            }

            promise_type* promise;
        };

        struct promise_type {

            template<typename ThisT, typename AwaitableT>
            promise_type(ThisT this_, CoroutineThreadPool& pool, AwaitableT&): pool(&pool) {}

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
                exception = std::current_exception();
            }

            std::condition_variable cv;
            bool finished = false;
            std::mutex m;
            CoroutineThreadPool* pool;
            std::exception_ptr exception = nullptr;
        };

        void wait() {
            std::unique_lock l(promise->m);
            promise->cv.wait(l, [&]{return promise->finished;});
        }

        void get() {
            if (promise->exception) {
                std::rethrow_exception(promise->exception);
            }
        }

        run_sync(promise_type* promise): promise(promise) {}

        run_sync(const run_sync& o) = delete;
        run_sync& operator=(const run_sync& o) = delete;

        run_sync(run_sync&& o) {
            promise = o.promise;
            if constexpr (OwnHandle) {
                o.promise = nullptr;
            }
        }

        run_sync& operator=(run_sync&& o) {
            std::swap(o.promise, promise);
            return *this;
        }


        ~run_sync() {
            if constexpr (OwnHandle) {
                if (promise) {
                    std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
                }
            }
        }

        promise_type* promise;
    };


}

template<typename T>
struct AwaitTransformPassThrough {
    static constexpr bool pass_through = false;
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
            return_value_.template emplace<1>(std::forward<V>(v));
        }

        void unhandled_exception() {
            return_value_.template emplace<2>(std::current_exception());
        }

        template<typename AwaitableT, typename = std::enable_if_t<AwaitTransformPassThrough<std::decay_t<AwaitableT>>::pass_through>>
        decltype(auto) await_transform(AwaitableT&& awaitable) {
            return std::forward<AwaitableT>(awaitable);
        }

        template<typename AwaitableT, typename = std::enable_if_t<!AwaitTransformPassThrough<std::decay_t<AwaitableT>>::pass_through>>
        auto await_transform(AwaitableT&& awaitable) {
            using InnerT = promise::detail::ResultT<std::decay_t<AwaitableT>>;

            return [&awaitable](CoroutineThreadPool& pool) mutable -> promise::detail::wrapper_task<InnerT> {
                co_return co_await std::forward<AwaitableT>(awaitable);
            }(*pool);
        }

        std::coroutine_handle<> continuation;
        CoroutineThreadPool* pool;
        std::variant<std::monostate, T, std::exception_ptr> return_value_;
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
            switch (promise->return_value_.index()) {
                case 1: {
                    if constexpr (MoveOnResume) {
                        return std::move(std::get<1>(promise->return_value_));
                    } else {
                        return std::get<1>(promise->return_value_);
                    }
                }
                case 2: {
                    std::rethrow_exception(std::get<2>(promise->return_value_));
                }
                case 0: {
                    break;
                }
            };
            assert(false);
        }

        promise_type* promise;
    };

    constexpr auto operator co_await() & noexcept {
        return awaitable<false>{promise};
    }

    constexpr auto operator co_await() && noexcept {
        return awaitable<true>{promise};
    }

    Task(promise_type* promise): promise(promise) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& o) {
        promise = o.promise;
        o.promise = nullptr;
    }

    Task& operator=(Task&& o) {
        std::swap(o.promise, promise);
        return *this;
    }

    ~Task() {
        if (promise) {
            std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
        }
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

        void return_void() {}

        void unhandled_exception() {
            exception = std::current_exception();
        }

        template<typename AwaitableT, typename = std::enable_if_t<AwaitTransformPassThrough<std::decay_t<AwaitableT>>::pass_through>>
        decltype(auto) await_transform(AwaitableT&& awaitable) {
            return std::forward<AwaitableT>(awaitable);
        }

        template<typename AwaitableT, typename = std::enable_if_t<!AwaitTransformPassThrough<std::decay_t<AwaitableT>>::pass_through>>
        auto await_transform(AwaitableT&& awaitable) {
            using InnerT = promise::detail::ResultT<std::decay_t<AwaitableT>>;

            return [&awaitable](CoroutineThreadPool& pool) mutable -> promise::detail::wrapper_task<InnerT> {
                co_return co_await std::forward<AwaitableT>(awaitable);
            }(*pool);
        }

        std::coroutine_handle<> continuation;
        CoroutineThreadPool* pool;
        std::exception_ptr exception = nullptr;

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

        void await_resume() {
            if (promise->exception != nullptr) {
                std::rethrow_exception(promise->exception);
            }
        }

        promise_type* promise;
    };

    constexpr auto operator co_await() noexcept {
        return awaitable{promise};
    }

    Task(promise_type* promise): promise(promise) {}

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    Task(Task&& o) {
        promise = o.promise;
        o.promise = nullptr;
    }

    Task& operator=(Task&& o) {
        std::swap(o.promise, promise);
        return *this;
    }

    ~Task() {
        if (promise) {
            std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
        }
    }

    promise_type* promise;
};


template<typename T>
struct AwaitTransformPassThrough<Task<T>> {
    static constexpr bool pass_through = true;
};


template<typename A>
auto run_awaitable_sync(CoroutineThreadPool& pool, A a) {
    using T = promise::detail::ResultT<A>;

    promise::detail::run_sync coro = [&](CoroutineThreadPool& pool) -> promise::detail::run_sync<T, true> {
        if constexpr (std::is_void_v<T>) {
            co_await std::move(a);
        } else {
            co_return co_await std::move(a);
        }
    }(pool);
    coro.wait();
    if constexpr (std::is_void_v<T>) {
        std::move(coro).get();
        return;
    } else {
        return std::move(coro).get();
    }
}

template<typename F>
auto run_sync(CoroutineThreadPool& pool, F&& f) {
    return run_awaitable_sync(pool, f());
}

template<typename A>
void run_awaitable_async(CoroutineThreadPool& pool, A a) {
    using T = promise::detail::ResultT<A>;

    [](CoroutineThreadPool& pool, A a) mutable -> promise::detail::run_sync<T, false> {
        if constexpr (std::is_void_v<T>) {
            co_await std::move(a);
        } else {
            co_return co_await std::move(a);
        }
    }(pool, std::move(a));
}

template<typename F>
void run_async(CoroutineThreadPool& pool, F f) {
    run_awaitable_async(pool, f());
}



}