#pragma once

// depend on glibc
#include <endian.h>   // leXXtoh, beXXtoh
#include <byteswap.h> // bswap16, bswap32, bswap64
#include <cstdint>
#include <cstring>
#include <iostream>
#include <optional>

#include "fmt/ostream.h"

namespace coral {
    template<typename T>
    T decode(const T &value, bool isLittle);

    template<>
    inline uint8_t decode<uint8_t>(const uint8_t &v, bool isLittle) {
        return v;
    }

    template<>
    inline int8_t decode<int8_t>(const int8_t &v, bool isLittle) {
        return v;
    }

    template<>
    inline uint16_t decode<uint16_t>(const uint16_t &v, bool isLittle) {
        return isLittle ? le16toh(v) : be16toh(v);
    }

    template<>
    inline uint32_t decode<uint32_t>(const uint32_t &v, bool isLittle) {
        return isLittle ? le32toh(v) : be32toh(v);
    }

    template<>
    inline uint64_t decode<uint64_t>(const uint64_t &v, bool isLittle) {
        return isLittle ? le64toh(v) : be64toh(v);
    }

    template<>
    inline int16_t decode<int16_t>(const int16_t &v, bool isLittle) {
        return static_cast<int16_t>(
            decode<uint16_t>(static_cast<const uint16_t &>(v), isLittle)
        );
    }

    template<>
    inline int32_t decode<int32_t>(const int32_t &v, bool isLittle) {
        return static_cast<int32_t>(
            decode<uint32_t>(static_cast<const uint32_t &>(v), isLittle)
        );
    }

    template<>
    inline int64_t decode<int64_t>(const int64_t &v, bool isLittle) {
        return static_cast<int64_t>(
            decode<uint64_t>(static_cast<const uint64_t &>(v), isLittle)
        );
    }

    inline bool is_fill_pattern(const uint32_t val) noexcept {
        const uint32_t pattern = (val & 0xFF) * 0x01010101U;
        return val == pattern;
    }

    /** Fill pattern(오라클 정크 패딩) (ASCII 0x20 ~ 0x7E 범위의 반복 문자)
     * 'AAAA', 'BBBB', 'CCCC', 'DDDD', 'FFFF' ... */
    inline bool is_ascii_filler(const uint32_t val) noexcept {
        const auto b = static_cast<uint8_t>(val & 0xFF);
        return (b >= 0x20 && b <= 0x7E) && is_fill_pattern(val);
    }

    /** * Fill pattern */
    inline uint32_t sanitize_filler(const int32_t val) noexcept {
        if (is_ascii_filler(val)) {
            return 0;
        }
        return val;
    }

    using std::cerr;
    using fmt::println;

    inline uint64_t get_u64( const tcb::span<const char>& s, const size_t offset, const bool isLittle) {
        uint64_t value = 0;
        std::memcpy(&value, s.data() + offset, sizeof(uint64_t));
        return decode(value, isLittle);
    }

    inline std::optional<uint64_t> read_u64(const tcb::span<const char> &s,
                                            const size_t offset, const bool isLittle) noexcept {
        if (offset + sizeof(uint64_t) > s.size()) {
            println(cerr, "OUT-OF-RANGE: read_ui64: offset out of bounds {}", offset);
            return std::nullopt;
        }
        return get_u64(s, offset, isLittle);
    }


    inline uint32_t get_u32(const tcb::span<const char> &s,
                            const size_t offset, const bool isLittle) {
        uint32_t value = 0;
        std::memcpy(&value, s.data() + offset, sizeof(uint32_t));
        return decode(value, isLittle);
    }


    inline std::optional<uint32_t> read_u32(
        const tcb::span<const char>& s,
        const size_t offset,
        const bool isLittle) noexcept {
        if (offset + sizeof(uint32_t) > s.size()) {
            println(cerr, "OUT-OF-RANGE: read_ui32: offset out of bounds {}", offset);
            return std::nullopt;
        }

        return get_u32(s, offset, isLittle);
    }

    inline uint16_t get_u16(const tcb::span<const char> &s,
                            const size_t offset, const bool isLittle) {
        uint16_t value = 0;
        std::memcpy(&value, s.data() + offset, sizeof(uint16_t));
        return decode(value, isLittle);
    }

    inline std::optional<uint16_t> read_u16(const tcb::span<const char> &s,
                                            const size_t offset, const bool isLittle) noexcept {
        if (offset + sizeof(uint16_t) > s.size()) {
            println(cerr, "OUT-OF-RANGE: read_ui16: offset out of bounds {}", offset);
            return std::nullopt;
        }

        uint16_t value = 0;
        std::memcpy(&value, s.data() + offset, sizeof(uint16_t));
        return decode(value, isLittle);
    }

    inline uint8_t get_u8( const tcb::span<const char>& s, const size_t offset) noexcept {
        return static_cast<uint8_t>(s[offset]);
    }
}

