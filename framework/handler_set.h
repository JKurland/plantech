#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

#include "thread_pool/promise.h"

namespace pt {

namespace handler_set::detail {
    template<size_t NewI, size_t...Is>
    constexpr std::index_sequence<Is..., NewI> append_index_impl(std::index_sequence<Is...>);

    template<typename T, size_t NewI>
    using append_index = decltype(append_index_impl<NewI>(std::declval<T>()));

    template<typename Pred>
    struct index_sequence_pred {};

    template<typename T>
    struct index_sequence_builder {};

    template<size_t I, typename Seq, typename Pred>
    struct index_sequence_accumulator {
        static constexpr Seq value = Seq{};
    };

    template<typename Pred, typename T>
    constexpr auto operator<<(index_sequence_pred<Pred>, index_sequence_builder<T>) {
        if constexpr (Pred::template value<T>) {
            return index_sequence_accumulator<1, std::index_sequence<0>, Pred>{};
        } else {
            return index_sequence_accumulator<1, std::index_sequence<>, Pred>{};
        }
    }

    template<typename Pred, typename T, typename Seq, size_t I>
    constexpr auto operator<<(index_sequence_accumulator<I, Seq, Pred>, index_sequence_builder<T>) {
        if constexpr (Pred::template value<T>) {
            return index_sequence_accumulator<I+1, append_index<Seq, I>, Pred>{};
        } else {
            return index_sequence_accumulator<I+1, Seq, Pred>{};
        }
    }

    template<typename Pred, typename...Ts>
    constexpr auto filtered_indexes() {
        return (index_sequence_pred<Pred>{} << ... << index_sequence_builder<Ts>{}).value;
    }

}

template<typename...HandlerTs>
class HandlerSet {
public:
    HandlerSet() = default;

    template<typename...ArgTs>
    HandlerSet(ArgTs&&...args): handlers(std::forward<ArgTs>(args)...) {}


    template<typename Pred>
    static constexpr auto true_indexes() {
        return handler_set::detail::filtered_indexes<Pred, HandlerTs...>();
    }

    template<size_t I>
    auto& get() {
        return std::get<I>(handlers);
    }

    template<typename F, size_t...Is>
    decltype(auto) call_with(std::index_sequence<Is...>, F&& f) {
        return std::invoke(std::forward<F>(f), std::get<Is>(handlers)...);
    }

private:
    std::tuple<HandlerTs...> handlers;
};

template<typename...ArgTs>
using HandlerSetFromArgs = HandlerSet<std::decay_t<ArgTs>...>;

template<typename...ArgTs>
HandlerSet(ArgTs...) -> HandlerSetFromArgs<ArgTs...>;


}
