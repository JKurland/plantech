#pragma once

#include <type_traits>
#include <utility>

namespace pt {

namespace ordered_destructing_tuple::detail {
    constexpr size_t pad_to(size_t size, size_t align) {
        if (size % align == 0) {
            return size;
        } else {
            return size + (align - size % align);
        }
    }

    template<size_t N, typename...Ts>
    struct nth_type {};

    template<size_t N, typename First, typename...Rest>
    struct nth_type<N, First, Rest...> {
        using type = typename nth_type<N-1, Rest...>::type;
    };

    template<typename First, typename...Rest>
    struct nth_type<0, First, Rest...> {
        using type = First;
    };

    template<size_t N, typename...Ts>
    using nth_type_t = typename nth_type<N, Ts...>::type;

    template<size_t N, typename Seq=std::index_sequence<>>
    struct make_reverse_index_sequence;

    template<size_t N, size_t...Is>
    struct make_reverse_index_sequence<N, std::index_sequence<Is...>> {
        using type = typename make_reverse_index_sequence<N-1, std::index_sequence<Is..., N-1>>::type;
    };

    template<size_t...Is>
    struct make_reverse_index_sequence<0, std::index_sequence<Is...>> {
        using type = std::index_sequence<Is...>;
    };

    template<size_t N>
    using make_reverse_index_sequence_t = typename make_reverse_index_sequence<N>::type;
}

template<typename...Ts>
class OrderedDestructingTuple {
private:
    template<typename ...ArgTs>
    static constexpr bool enable_forwarding_ctor();

    template<size_t...Is>
    static constexpr bool enable_default_ctor(std::index_sequence<Is...>);

public:
    template<typename...ArgTs, typename = std::enable_if_t<enable_forwarding_ctor<ArgTs...>()>>
    OrderedDestructingTuple(ArgTs&&...args);

    OrderedDestructingTuple();

    OrderedDestructingTuple(const OrderedDestructingTuple& o);
    OrderedDestructingTuple(OrderedDestructingTuple&& o);

    // don't need operator= yet, do them later
    OrderedDestructingTuple& operator=(const OrderedDestructingTuple& o);
    OrderedDestructingTuple& operator=(OrderedDestructingTuple&& o);

    ~OrderedDestructingTuple();

    template<size_t I>
    ordered_destructing_tuple::detail::nth_type_t<I, Ts...>& get();

    template<size_t I>
    const ordered_destructing_tuple::detail::nth_type_t<I, Ts...>& get() const;

    template<typename...OtherTs>
    OrderedDestructingTuple<Ts..., OtherTs...> concat(const OrderedDestructingTuple<OtherTs...>& o) &;

    template<typename...OtherTs>
    OrderedDestructingTuple<Ts..., OtherTs...> concat(const OrderedDestructingTuple<OtherTs...>& o) &&;

    template<typename...OtherTs>
    OrderedDestructingTuple<Ts..., OtherTs...> concat(OrderedDestructingTuple<OtherTs...>&& o) &;

    template<typename...OtherTs>
    OrderedDestructingTuple<Ts..., OtherTs...> concat(OrderedDestructingTuple<OtherTs...>&& o) &&;
private:
    static constexpr std::array<size_t, sizeof...(Ts)> sizes();
    static constexpr std::array<size_t, sizeof...(Ts)> alignments();
    static constexpr std::array<size_t, sizeof...(Ts)> offsets();

    static constexpr size_t total_size();
    static constexpr size_t total_alignment();


    template<size_t...Is, typename...ArgTs>
    void init(std::index_sequence<Is...>, ArgTs&&...args);

    template<size_t Offset = 0, size_t...Is, typename...OTs>
    void copy_init(std::index_sequence<Is...>, const OrderedDestructingTuple<OTs...>& o);

    template<size_t Offset = 0, size_t...Is, typename...OTs>
    void move_init(std::index_sequence<Is...>, OrderedDestructingTuple<OTs...>&& o);

    template<size_t...Is, typename = std::enable_if_t<enable_default_ctor(std::index_sequence<Is...>{})>>
    void default_init(std::index_sequence<Is...>);

    template<size_t...Is>
    void deinit(std::index_sequence<Is...>);

    template<size_t I>
    void deinit_i();

    alignas(total_alignment()) unsigned char storage[total_size()];

    template<typename...OtherTs>
    friend class OrderedDestructingTuple;

    // when constructing the new tuple in concat we don't want to init
    struct UninitFlag {};
    OrderedDestructingTuple(UninitFlag) {}
};

template<typename...ArgTs>
OrderedDestructingTuple(ArgTs...args) -> OrderedDestructingTuple<std::decay_t<ArgTs>...>;

template<typename...Ts>
OrderedDestructingTuple<Ts...>::OrderedDestructingTuple() {
    default_init(std::make_index_sequence<sizeof...(Ts)>{});
}

template<typename...Ts>
template<size_t...Is, typename>
void OrderedDestructingTuple<Ts...>::default_init(std::index_sequence<Is...>) {
    (new (reinterpret_cast<Ts*>( &storage[offsets()[Is]] )) Ts() , ...);
}

template<typename...Ts>
template<typename...ArgTs, typename>
OrderedDestructingTuple<Ts...>::OrderedDestructingTuple(ArgTs&&...args) {
    init(std::make_index_sequence<sizeof...(Ts)>{}, std::forward<ArgTs>(args)...);
}

template<typename...Ts>
template<size_t...Is, typename...ArgTs>
void OrderedDestructingTuple<Ts...>::init(std::index_sequence<Is...>, ArgTs&&...args) {
    (new (reinterpret_cast<Ts*>( &storage[offsets()[Is]] )) Ts(std::forward<ArgTs>(args)), ...);
}

template<typename...Ts>
OrderedDestructingTuple<Ts...>::OrderedDestructingTuple(const OrderedDestructingTuple& o) {
    copy_init(std::make_index_sequence<sizeof...(Ts)>{}, o);
}

template<typename...Ts>
OrderedDestructingTuple<Ts...>& OrderedDestructingTuple<Ts...>::operator=(const OrderedDestructingTuple& o) {
    if (this == &o) return *this;
    copy_init(std::make_index_sequence<sizeof...(Ts)>{}, std::move(o));
    return *this;
}

template<typename...Ts>
template<size_t Offset, size_t...Is, typename...OTs>
void OrderedDestructingTuple<Ts...>::copy_init(std::index_sequence<Is...>, const OrderedDestructingTuple<OTs...>& o) {
    (new (reinterpret_cast<OTs*>( &storage[offsets()[Is + Offset]] )) OTs(o.template get<Is>()), ...);
}

template<typename...Ts>
OrderedDestructingTuple<Ts...>::OrderedDestructingTuple(OrderedDestructingTuple&& o) {
    move_init(std::make_index_sequence<sizeof...(Ts)>{}, std::move(o));
}

template<typename...Ts>
OrderedDestructingTuple<Ts...>& OrderedDestructingTuple<Ts...>::operator=(OrderedDestructingTuple&& o) {
    if (this == &o) return *this;
    move_init(std::make_index_sequence<sizeof...(Ts)>{}, std::move(o));
    return *this;
}

template<typename...Ts>
template<size_t Offset, size_t...Is, typename...OTs>
void OrderedDestructingTuple<Ts...>::move_init(std::index_sequence<Is...>, OrderedDestructingTuple<OTs...>&& o) {
    (new (reinterpret_cast<OTs*>( &storage[offsets()[Is+Offset]] )) OTs(std::move(o.template get<Is>())), ...);
}

template<typename...Ts>
OrderedDestructingTuple<Ts...>::~OrderedDestructingTuple() {
    deinit(ordered_destructing_tuple::detail::make_reverse_index_sequence_t<sizeof...(Ts)>{});
}

template<typename...Ts>
template<size_t...Is>
void OrderedDestructingTuple<Ts...>::deinit(std::index_sequence<Is...>) {
    (deinit_i<Is>(), ...);
}

template<typename...Ts>
template<size_t I>
void OrderedDestructingTuple<Ts...>::deinit_i() {
    using T = ordered_destructing_tuple::detail::nth_type_t<I, Ts...>;
    get<I>().~T();
}

template<typename...Ts>
template<size_t I>
ordered_destructing_tuple::detail::nth_type_t<I, Ts...>& OrderedDestructingTuple<Ts...>::get() {
    return *reinterpret_cast<ordered_destructing_tuple::detail::nth_type_t<I, Ts...>*>(std::launder(&storage[offsets()[I]]));
}


template<typename...Ts>
template<size_t I>
const ordered_destructing_tuple::detail::nth_type_t<I, Ts...>& OrderedDestructingTuple<Ts...>::get() const {
    return *reinterpret_cast<const ordered_destructing_tuple::detail::nth_type_t<I, Ts...>*>(std::launder(&storage[offsets()[I]]));
}

template<typename...Ts>
template<typename...OtherTs>
OrderedDestructingTuple<Ts..., OtherTs...> OrderedDestructingTuple<Ts...>::concat(const OrderedDestructingTuple<OtherTs...>& o) & {
    using ResultT = OrderedDestructingTuple<Ts..., OtherTs...>;

    ResultT result(typename ResultT::UninitFlag{});
    result.copy_init(std::make_index_sequence<sizeof...(Ts)>{}, *this);
    result.template copy_init<sizeof...(Ts)>(std::make_index_sequence<sizeof...(OtherTs)>{}, o);

    return result;
}

template<typename...Ts>
template<typename...OtherTs>
OrderedDestructingTuple<Ts..., OtherTs...> OrderedDestructingTuple<Ts...>::concat(const OrderedDestructingTuple<OtherTs...>& o) && {
    using ResultT = OrderedDestructingTuple<Ts..., OtherTs...>;

    ResultT result(typename ResultT::UninitFlag{});
    result.move_init(std::make_index_sequence<sizeof...(Ts)>{}, std::move(*this));
    result.template copy_init<sizeof...(Ts)>(std::make_index_sequence<sizeof...(OtherTs)>{}, o);

    return result;
}

template<typename...Ts>
template<typename...OtherTs>
OrderedDestructingTuple<Ts..., OtherTs...> OrderedDestructingTuple<Ts...>::concat(OrderedDestructingTuple<OtherTs...>&& o) & {
    using ResultT = OrderedDestructingTuple<Ts..., OtherTs...>;

    ResultT result(typename ResultT::UninitFlag{});
    result.copy_init(std::make_index_sequence<sizeof...(Ts)>{}, *this);
    result.template move_init<sizeof...(Ts)>(std::make_index_sequence<sizeof...(OtherTs)>{}, std::move(o));

    return result;
}

template<typename...Ts>
template<typename...OtherTs>
OrderedDestructingTuple<Ts..., OtherTs...> OrderedDestructingTuple<Ts...>::concat(OrderedDestructingTuple<OtherTs...>&& o) && {
    using ResultT = OrderedDestructingTuple<Ts..., OtherTs...>;

    ResultT result(typename ResultT::UninitFlag{});
    result.move_init(std::make_index_sequence<sizeof...(Ts)>{}, std::move(*this));
    result.template move_init<sizeof...(Ts)>(std::make_index_sequence<sizeof...(OtherTs)>{}, std::move(o));

    return result;
}

template<typename...Ts>
constexpr std::array<size_t, sizeof...(Ts)> OrderedDestructingTuple<Ts...>::sizes() {
    return std::array<size_t, sizeof...(Ts)>{sizeof(Ts)...};
}

template<typename...Ts>
constexpr std::array<size_t, sizeof...(Ts)> OrderedDestructingTuple<Ts...>::alignments() {
    return std::array<size_t, sizeof...(Ts)>{alignof(Ts)...};
}

template<typename...Ts>
constexpr std::array<size_t, sizeof...(Ts)> OrderedDestructingTuple<Ts...>::offsets() {
    std::array<size_t, sizeof...(Ts)> offsets{};

    if (sizeof...(Ts) > 0) {
        offsets[0] = 0;
    }

    for (size_t i = 1; i < sizeof...(Ts); i++) {
        offsets[i] = ordered_destructing_tuple::detail::pad_to(offsets[i-1] + sizes()[i-1], alignments()[i]);
    }

    return offsets;
}

template<typename...Ts>
constexpr size_t OrderedDestructingTuple<Ts...>::total_size() {
    if (sizeof...(Ts) == 0) {
        return 1;
    } else {
        return offsets().back() + sizes().back();
    }
}

template<typename...Ts>
constexpr size_t OrderedDestructingTuple<Ts...>::total_alignment() {
    size_t max_align = 1;
    for (size_t a : alignments()) {
        if (max_align % a != 0) {
            size_t old_align = max_align;
            // alignments are always small, just increment until we find a good value.
            while (max_align % old_align != 0 || max_align % a != 0) {
                max_align++;
            }
        }
    }
    return max_align;
}

template<typename...Ts>
template<typename ...ArgTs>
constexpr bool OrderedDestructingTuple<Ts...>::enable_forwarding_ctor() {
    if (sizeof...(Ts) == 0) return false;
    if constexpr (sizeof...(Ts) != sizeof...(ArgTs)) {
        return false;
    } else {
        return (std::is_constructible_v<Ts, ArgTs> && ...);
    }
}

template<typename...Ts>
template<size_t...Is>
constexpr bool OrderedDestructingTuple<Ts...>::enable_default_ctor(std::index_sequence<Is...>) {
    // the index sequence is stupid, just ignore it. It allows default_init to force template subsititution so that
    // sfinae actually happens.
    return (std::is_default_constructible_v<Ts> && ...);
}

}