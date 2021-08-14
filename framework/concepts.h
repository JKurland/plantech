#pragma once
#include <concepts>
#include "thread_pool/promise.h"

namespace pt {

template<typename T>
concept Event = true;

template<typename T>
concept Response = true;

template<typename T>
concept Request = requires(T t) { typename T::ResponseT; } && Response<typename T::ResponseT>;


template<typename T>
concept IsContext = true;


template<typename T, typename C, typename E>
concept HandlesEvent = IsContext<C> && Event<E> && requires(T t, C& c, const E& e) {
    {t.handle(c, e)} -> std::same_as<Task<>>;
};

template<typename T, typename C, typename R>
concept HandlesRequest = IsContext<C> && Request<R> && requires(T t, C& c, const R& r) {
    {t.handle(c, r)} -> std::same_as<Task<typename R::ResponseT>>;
};

}
