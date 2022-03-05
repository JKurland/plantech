#pragma once

#include <variant>
#include <cassert>

namespace pt::msg_lang {
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
};

}