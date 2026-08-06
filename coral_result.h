#pragma once
#include <optional>
#include <string>

namespace coral {
    template <typename T>
    struct Result {
        enum class Kind { Ok, Error };

        Kind kind;
        std::optional<T> value;
        std::string error;

        static Result Ok(T v) { return {Kind::Ok, std::move(v), {}}; }
        static Result Error(std::string e) { return {Kind::Error, std::nullopt, std::move(e)}; }

        bool ok() const { return value.has_value(); }

        const T& get() const { return value.value(); }
        T& get() { return value.value(); }

    };

    template<typename T>
    Result<T> Err_of(const std::string& msg) {
        return Result<T>::Error(std::move(msg));
    }

    // lvalue: copy, rvalue: move
    template<typename T>
    Result<std::decay_t<T>> Ok_of(T&& value) {
        return Result<std::decay_t<T>>::Ok(std::forward<T>(value));
    }
}

