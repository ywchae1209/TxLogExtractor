#pragma once
#include <string>
#include <tl/expected.hpp>

namespace coral {

    template <typename T>
    using Result = tl::expected<T, std::string>;

    inline auto err_of(std::string err) {
        return tl::make_unexpected(std::move(err));
    }
}

