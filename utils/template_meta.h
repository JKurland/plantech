#pragma once

#include <type_traits>
#include <utility>

namespace pt {

namespace template_meta::detail {
    template<typename T, typename...Ts, size_t...Is>
    constexpr size_t ppFindType(std::index_sequence<Is...>) {
        return ((std::is_same_v<T, Ts> ? Is : 0) + ...);
    }

    template<typename First, typename...Ts>
    struct PpFirstType {
        using type = First;
    };
}

template<typename T, typename...Ts>
constexpr size_t ppCountType() {
    return (std::is_same_v<T, Ts> + ... + 0);
}

template<typename T, typename...Ts>
constexpr bool ppContainsType() {
    return (std::is_same_v<T, Ts> || ...);
}

template<typename T, typename...Ts>
constexpr size_t ppFindType() {
    static_assert(ppCountType<T, Ts...>() == 1, "Ts does not contain exactly 1 T");
    return template_meta::detail::ppFindType<T, Ts...>(std::make_index_sequence<sizeof...(Ts)>{});
}

template<typename...Ts>
using PpFirstType = typename template_meta::detail::PpFirstType<Ts...>::type;

}