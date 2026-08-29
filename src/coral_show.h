#pragma once

#define FMT_HEADER_ONLY 1
#include <iostream>
#include <sstream>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fmt/ostream.h>
#include <tcb/span.hpp>
#include <string>
#include <array>
#include <algorithm>
#include <limits>

namespace coral {

    constexpr auto Rev_st = "\033[7m";
    constexpr auto Rev_End = "\033[27m";

    constexpr auto reset_color  = "\033[0m ";

    constexpr auto red      = "\033[31m"; // green
    constexpr auto green    = "\033[32m"; // green
    constexpr auto yellow   = "\033[33m"; // yellow
    constexpr auto blue     = "\033[34m"; // yellow
    constexpr auto magenta  = "\033[35m"; // magenta
    constexpr auto cyan     = "\033[36m"; // cyan

    constexpr auto b_red    = "\033[91m"; // green
    constexpr auto b_green  = "\033[92m";
    constexpr auto b_yellow = "\033[93m";
    constexpr auto b_blue   = "\033[94m";
    constexpr auto b_magenta= "\033[95m";
    constexpr auto b_cyan   = "\033[96m";
    constexpr auto b_white  = "\033[97m";

    constexpr auto B_red    = "\033[1;91m"; // green
    constexpr auto B_green  = "\033[1;92m";
    constexpr auto B_yellow = "\033[1;93m";
    constexpr auto B_blue   = "\033[1;94m";
    constexpr auto B_magenta= "\033[1;95m";
    constexpr auto B_cyan   = "\033[1;96m";
    constexpr auto B_white  = "\033[1;97m";

    constexpr auto key_color = B_green;
    constexpr auto desc_color= yellow;

    constexpr const char *colors[] = {
        b_cyan,
        b_green,
        b_blue,
        b_magenta
    };

    inline void show_HexDump(const tcb::span<const char> &raw, std::ostream& os = std::cout, size_t rows = 8) {

        static constexpr std::array<char[2], 256> hex_lut = []() {
            std::array<char[2], 256> lut{};
            constexpr char hex_digits[] = "0123456789ABCDEF";
            for (int i = 0; i < 256; ++i) {
                lut[i][0] = hex_digits[(i >> 4) & 0x0F];
                lut[i][1] = hex_digits[i & 0x0F];
            }
            return lut;
        }();

        constexpr size_t colors_sz = 4;

        size_t max_groups = rows * 8;           //
        const size_t total_groups = raw.size() / 2;
        const auto odd = raw.size() % 2 == 1;

        const size_t limit = std::min(total_groups, max_groups);

        os << "  ---------------------------------------\n";
        if (raw.empty()) return;

        size_t n = 0;
        os << "  ";
        for (size_t i = 0; i < limit * 2; i += 2) {

            os << colors[n & (colors_sz - 1)];
            const auto b1 = static_cast<uint8_t>(raw[i]);
            const auto b2 = static_cast<uint8_t>(raw[i + 1]);
            os.write(hex_lut[b1], 2);
            os.write(hex_lut[b2], 2);
            os.write(reset_color, 5);

            n++;
            if ((n & 7) == 0) {
                os << (n < limit
                           ? "\n  "
                           : (i + 2 >= limit * 2) && !odd
                                 ? "\n"
                                 : "\n  ");
            }
        }

        if (odd && n < max_groups) {
            const auto last = static_cast<uint8_t>(raw.back());
            os << colors[n & (colors_sz - 1)];
            os.write(hex_lut[last], 2);
            os.write(reset_color, 5);
            os << "\n";
            return;
        }

        if ((n & 7) != 0 ) {
            os.put('\n');
        }
    }


    // bytes
    inline std::string toHex(const uint8_t* data, const size_t len) {
        return fmt::format("{:02x}", fmt::join(data, data + len, ""));
    }

    // byte container == std::string_view, std::vector...
    template <typename Container>
    std::string toHex(const Container& container, bool with_prefix = true) {
        return with_prefix
                   ? fmt::format("0x{:02x}", fmt::join(container, ""))
                   : fmt::format("{:02x}", fmt::join(container, ""));
    }

    inline std::string toHex(uint64_t val, const bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:016x}", val) : fmt::format("{:016x}", val);
    }

    inline std::string toHex(uint32_t val, const bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:08x}", val) : fmt::format("{:08x}", val);
    }

    inline std::string toHex(uint16_t val, const bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:04x}", val) : fmt::format("{:04x}", val);
    }

    inline std::string toHex(uint8_t val, const bool with_prefix = true) {
        return with_prefix ? fmt::format("0x{:02x}", val) : fmt::format("{:02x}", val);
    }
}
