#pragma once

#define FMT_HEADER_ONLY 1
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/ostream.h>


namespace coral {

    // bytes
    inline std::string toHex(const uint8_t* data, size_t len) {
        return fmt::format("{:02x}", fmt::join(data, data + len, ""));
    }

    // byte-container :: std::string_view, std::vector...
    template <typename Container>
    inline std::string toHex(const Container& container, bool with_prefix = true) {
        return with_prefix
                   ? fmt::format("0x{:02x}", fmt::join(container, ""))
                   : fmt::format("{:02x}", fmt::join(container, ""));
    }

    // uint64_t
    inline std::string toHex(uint64_t val, bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:016x}", val) : fmt::format("{:016x}", val);
    }

    // uint32_t
    inline std::string toHex(uint32_t val, bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:08x}", val) : fmt::format("{:08x}", val);
    }

    // uint16_t
    inline std::string toHex(uint16_t val, bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:04x}", val) : fmt::format("{:04x}", val);
    }

    // uint8_t
    inline std::string toHex(uint8_t val, bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:02x}", +val) : fmt::format("{:02x}", +val);
    }
}
