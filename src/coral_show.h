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

    inline void show_HexDump(const tcb::span<const char> &raw, size_t max_rows = 8) {

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

        size_t max_groups = std::numeric_limits<size_t>::max();
        if (max_rows > 0) {
            if (max_rows > std::numeric_limits<size_t>::max() / 8) {
                max_groups = std::numeric_limits<size_t>::max();
            } else {
                max_groups = max_rows * 8;
            }
        }

        const size_t total_groups = raw.size() / 2;
        const size_t limit = std::min(total_groups, max_groups);

        std::cout << "---------------------------------------\n";

        size_t n = 0;
        for (size_t i = 0; i < limit * 2; i += 2) {
            uint8_t b1 = static_cast<uint8_t>(raw[i]);
            uint8_t b2 = static_cast<uint8_t>(raw[i + 1]);

            std::cout << colors[n & (colors_sz - 1)];

            std::cout.write(hex_lut[b1], 2);
            std::cout.write(hex_lut[b2], 2);

            std::cout.write(reset_color, 5);

            n++;
            if ((n & 7) == 0) {
                std::cout.put('\n');
            }
        }

        bool odd_processed = false;
        if (raw.size() % 2 == 1 && n < max_groups) {
            const uint8_t last = static_cast<uint8_t>(raw.back());

            std::cout << colors[n & (colors_sz - 1)];
            std::cout.write(hex_lut[last], 2);
            std::cout.write(reset_color, 5);
            odd_processed = true;
        }
        if ((n & 7) != 0 || odd_processed) {
            std::cout.put('\n');
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
