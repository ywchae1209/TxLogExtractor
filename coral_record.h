#pragma once
#include <cstdint>
#include <optional>
#include <tcb/span.hpp>

#include "coral_decode.h"
#include "coral_show.h"

#include "RedoHead.h"

namespace coral {

    inline std::optional<uint32_t> read_u32(
        const tcb::span<const char> s,
        const size_t offset,
        const bool isLittle) noexcept {

        if (offset + sizeof(uint32_t) > s.size()) {
            fmt::println("OUT-OF-RANGE: read_ui32: offset out of bounds {}", offset);
            return std::nullopt;
        }

        uint32_t value = 0;
        std::memcpy(&value, s.data() + offset, sizeof(uint32_t));

        return decode(value, isLittle);
    }

    inline std::optional<uint16_t> read_u16(
        const tcb::span<const char> s,
        const size_t offset,
        const bool isLittle) noexcept {

        if (offset + sizeof(uint16_t) > s.size()) {
            fmt::println("OUT-OF-RANGE: read_ui16: offset out of bounds {}", offset);
            return std::nullopt;
        }

        uint16_t value = 0;
        std::memcpy(&value, s.data() + offset, sizeof(uint16_t));

        return decode(value, isLittle);
    }

    inline std::optional<uint8_t> read_u8(
        const tcb::span<const char> s,
        const size_t offset) noexcept {

        if (offset + sizeof(uint8_t) > s.size()) {
            fmt::println("OUT-OF-RANGE: read_u8: offset out of bounds {}", offset);
            return std::nullopt;
        }

        return static_cast<uint8_t>(s[offset]);
    }

    // from limited source.
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

    [[nodiscard]] inline bool dependBit_on(uint8_t vld) {
        return (vld & KCR_depend) == KCR_depend;
    }

    [[nodiscard]] inline std::string vld_string(uint8_t vld) {

        // 0x00은 비정상 / Zero Padding 구역
        if (vld == KCR_void) {
            return "void";
        }

        std::string result;
        result.reserve(64); // 불필요한 재할당 방지 메모리 예약

        auto append_flag = [&result](const char* flag_name) {
            if (!result.empty()) {
                result += '|';
            }
            result += flag_name;
        };

        if (vld & KCR_valid)    append_flag("valid");
        if (vld & KCR_commit)   append_flag("commit");
        if (vld & KCR_depend)   append_flag("depend");
        if (vld & KCR_newMark)  append_flag("nmark");
        if (vld & KCR_oldMark)  append_flag("omark");
        if (vld & KCR_gno)      append_flag("gno");
        if (vld & KCR_timeWrap) append_flag("timewrap");
        if (vld & KCR_raw)      append_flag("raw");

        return result;
    }

    inline bool is_valid_record_len( uint32_t l) noexcept {

        constexpr uint32_t MIN_RECORD = 16;
        const bool ok =
                (l & 3) == 0 &&      // 4의 배수
                l >= MIN_RECORD;

        return ok;
    }

    // 하위 5bit : 0x1F : 하나는 setting되어 있어야 함.
    constexpr uint8_t VLD_CORE_MASK = KCR_valid | KCR_commit | KCR_depend | KCR_newMark | KCR_oldMark;

    // new-mark(0x08)와 old-mark(0x10) 동시 포함 불가
    constexpr uint8_t VLD_MARK_CONFLICT_MASK = KCR_newMark | KCR_oldMark;

    inline bool validate_len_vld(const uint8_t v, const uint32_t l) noexcept {

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
    // un-official
    namespace FooFlags {
        constexpr uint8_t SUB_COMMIT     = 0x01; // Commit SCN Marker
        constexpr uint8_t SUB_ROLLBACK   = 0x02; // Rollback Marker
        constexpr uint8_t SUB_CKPT       = 0x04; // Checkpoint
        constexpr uint8_t SUB_HBKUP      = 0x08; // Hot Backup
        constexpr uint8_t SUB_DIRECT     = 0x10; // Direct-Path-Write
        constexpr uint8_t SUB_RECOVERY   = 0x20; // Media Recovery Control
        constexpr uint8_t SUB_MULTI_TRAN = 0x40; // Distributed / Multi-Transaction
        constexpr uint8_t SUB_SPECIAL    = 0x80; // Special System Record
    }

    [[nodiscard]] inline std::string foo_string(uint8_t foo) {
        // 0x00은 비트가 아무것도 안 켜진 일반 DML/DDL 레코드
        if (foo == 0x00) {
            return "normal";
        }

        std::string result;
        result.reserve(64);

        auto append_flag = [&result](const char* flag_name) {
            if (!result.empty()) {
                result += '|';
            }
            result += flag_name;
        };

        if (foo & FooFlags::SUB_COMMIT)     append_flag("commit");
        if (foo & FooFlags::SUB_ROLLBACK)   append_flag("rollback");
        if (foo & FooFlags::SUB_CKPT)       append_flag("ckpt");
        if (foo & FooFlags::SUB_HBKUP)      append_flag("hbkup");
        if (foo & FooFlags::SUB_DIRECT)     append_flag("direct");
        if (foo & FooFlags::SUB_RECOVERY)   append_flag("recovery");
        if (foo & FooFlags::SUB_MULTI_TRAN) append_flag("multi_tran");
        if (foo & FooFlags::SUB_SPECIAL)    append_flag("special");

        return result;
    }
    // --------------------------------------------------------------------------------

    // --------------------------------------------------------------------------------
    inline std::optional<uint32_t> read_record_length_with_validation(
        const tcb::span<const char> view,
        const size_t offset,
        const bool isLittle,
        const ora::SCN& low,
        const ora::SCN& next )
    {

        if (offset == 0) return std::nullopt;
        if (offset < 16) return std::nullopt;

        // assumption must contain 12 bytes  ---  this is a strong assumption.
        if (offset > (view.size() - 12)) return std::nullopt;

        // validate length
        const auto len = read_u32(view, offset, isLittle);
        if ( !len || !is_valid_record_len(*len) ) return std::nullopt;

        // validate with VLD
        const auto vld = read_u8(view, offset + 4);
        if (vld && !validate_len_vld(*vld, *len)) return std::nullopt;

        // validate with wrap & base
        const auto wrap = read_u16(view, offset + 6, isLittle);
        const auto base = read_u32(view, offset + 8, isLittle);
        bool ok =
                low.wrap <= *wrap && *wrap <= next.wrap &&
                low.base <= *base && *base <= next.base;
        if (!ok) {
            // fmt::println("validate : {}.{} ? {} ~ {}", toHex(*wrap), toHex(*base, false), toHex(low), toHex(next));
            return std::nullopt;
        }

        // const auto foo = read_u8(view, offset + 5);
        // fmt::println("vld : {} \t foo : {}", vld_string(*vld),foo_string(*foo));

        return *len;
    }

}
