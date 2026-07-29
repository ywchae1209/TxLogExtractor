#pragma once

#include <endian.h>   // leXXtoh, beXXtoh
#include <byteswap.h> // bswap16, bswap32, bswap64
#include <optional>
#include <cstring>

#include <tcb/span.hpp>
#include "coral_show.h"

// depend on glibc
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

    inline std::optional<uint32_t> read_ui32(const tcb::span<const char> s, const size_t offset, const bool isLittle) {

        if (offset + sizeof(uint32_t) > s.size()) {
            fmt::println("OUT-OF-RANGE: read_ui32: offset out of bounds");
            return std::nullopt;
        }

        uint32_t value = 0;
        std::memcpy(&value, s.data() + offset, sizeof(uint32_t));

        return decode(value, isLittle);
    }

    inline std::optional<uint8_t> read_u8(const tcb::span<const char> s, const size_t offset) {

        if (offset + sizeof(uint8_t) > s.size()) {
            fmt::println("OUT-OF-RANGE: read_u8: offset out of bounds");
            return std::nullopt;
        }

        return static_cast<uint8_t>(s[offset]);
    }

    enum VldFlags : uint8_t {
        KCR_void  = 0x00, // Invalid / Padding
        KCR_valid = 0x01, // Valid record
        KCR_commit = 0x02, // Commit SCN
        KCR_depend = 0x04, // Dependent SCN
        KCR_newMark = 0x08, // New SCN mark
        KCR_oldMark = 0x10, // Old SCN mark
        KCR_gno   = 0x20, // Group/Thread
        KCR_timeWrap = 0x40, // Timewarp
        KCR_raw   = 0x80  // Raw Record
    };

    // 하위 5bit : 0x1F
    constexpr uint8_t VLD_CORE_MASK = KCR_valid | KCR_commit | KCR_depend | KCR_newMark | KCR_oldMark;

    // NMARK(0x08)와 OMARK(0x10) 동시 포함 불가
    constexpr uint8_t VLD_MARK_CONFLICT_MASK = KCR_newMark | KCR_oldMark;

    inline bool is_valid_record_len( uint32_t l) noexcept {

        constexpr uint32_t MIN_RECORD = 16;
        constexpr uint32_t MAX_RECORD = 32 * 1024 * 1024;
        const bool ok =
                (l & 3) == 0 &&      // 4의 배수
                l >= MIN_RECORD &&
                l <= MAX_RECORD ;

        return ok;
    }

    inline bool validate_len_vld(uint8_t v, uint32_t l) noexcept {

        const bool not_valid =
                v == KCR_void ||
                (v & VLD_CORE_MASK) == 0 ||
                (v & VLD_MARK_CONFLICT_MASK) == VLD_MARK_CONFLICT_MASK;

        if (not_valid) return false;

        // KCR_raw need KCR_valid
        if ((v & KCR_raw) != 0 && (v & KCR_valid) == 0) {
            return false;
        }

        constexpr uint32_t MIN_CHANGE = 24;
        constexpr uint32_t MAX_MARK = 1024;

        const bool has_change = (v & KCR_valid) != 0;
        const bool just_mark  = !has_change && (v & (KCR_commit | KCR_newMark | KCR_oldMark)) != 0;

        if (has_change) return l >= MIN_CHANGE;
        if (just_mark)  return l <= MAX_MARK;

        return true;

    }
    // --------------------------------------------------------------------------------

    inline std::optional<uint32_t> read_record_length( const tcb::span<const char> view,
                                                       const size_t offset,
                                                       const bool isLittle) {
        if (offset == 0) return std::nullopt;
        if (offset < 16) return std::nullopt;

        const auto len = read_ui32(view, offset, isLittle);
        if (!len)
            return std::nullopt;

        const auto l = len.value();

        if ( !is_valid_record_len(l) )
            return std::nullopt;

        const auto vld = read_u8(view, offset + 4);
        if (vld && !validate_len_vld(*vld, l))
            return std::nullopt;

        return l;
    }
}

