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

    constexpr const char *colors[] = {
        "\033[31m", // red
        "\033[32m", // green
        "\033[33m", // yellow
        "\033[36m" // cyan
    };
    constexpr const char *bright_green = "\033[92m";
    constexpr const char *reset_color = "\033[0m ";

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
