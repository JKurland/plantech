#pragma once

#include <variant>
#include <cassert>
#include <compare>

namespace pt::msg_lang {

namespace detail {
    template<typename ... Ts>                                                 
    struct Overload : Ts ... { 
        using Ts::operator() ...;
    };
    template<class... Ts> Overload(Ts...) -> Overload<Ts...>;
}

template<typename...Ts>
class MyVariant: public std::variant<Ts...> {
public:
    using std::variant<Ts...>::variant;

    template<typename T>
    bool is() const {
        return std::holds_alternative<T>(static_cast<const std::variant<Ts...>&>(*this));
    }

    template<typename T>
    T& get() {
        assert(is<T>());
        return std::get<T>(static_cast<std::variant<Ts...>&>(*this));
    };

    template<typename T>
    const T& get() const {
        assert(is<T>());
        return std::get<T>(static_cast<const std::variant<Ts...>&>(*this));
    };

    template<typename...Vs>
    auto visit(Vs...ts) const {
        return std::visit(detail::Overload{std::move(ts)...}, static_cast<std::variant<Ts...>>(*this));
    }

    template<typename...Vs>
    auto visit(Vs...ts) {
        return std::visit(detail::Overload{std::move(ts)...}, static_cast<std::variant<Ts...>>(*this));
    }

    constexpr auto operator<=>(const MyVariant&) const = default;
};


}