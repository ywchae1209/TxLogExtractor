#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>
#include <optional>
#include <variant>
#include <vector>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_combinator.h"

namespace ora {
    using coral::decode_at, coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector, std::variant;
    using namespace combinator;

    using coral::Result, coral::err_of;
    using namespace combinator;

    // ----------------------------------------------------------------------------------------------------
    namespace Kdx_code {
        constexpr uint8_t Purge1        = 0x02; // kdxlpu: purge leaf row
        constexpr uint8_t Purge2        = 0x03; // kdxlpu: purge leaf row
        constexpr uint8_t MarkDelete    = 0x04; // kdxlde: mark leaf row deleted
        constexpr uint8_t Restore       = 0x05; // kdxlre: restore leaf row (clear delete flags)
        constexpr uint8_t UpdateKeyData = 0x12; // kdxlup: update keydata in row
    };

    /// 5.1 ~ IdxUndo #2 --- pair with Layer 10.x Index Leaf Undo Header
    struct Kdxlk {
        uint8_t          code{0};        // Offset 0 : Sub-operation code
        uint8_t          itl{0};         // Offset 1 : ITL Slot
        uint8_t          kdxlkflg{0};    // Offset 2 : Index Lock Flag
        uint32_t         indexid{0};     // Offset 4 : Index Object ID
        uint32_t         block{0};       // Offset 8 : Target Leaf Block DBA
        int32_t          sdc{0};         // Offset 12: Sequence / Delta Count

        vector<uint16_t> key_sizes;      // Offset 24~: variable (키 개수 >= 1 시)

        std::string codeDesc() const {
            switch (code) {
                case Kdx_code::Restore: return "kdxlre: restore leaf row (clear delete flags)"; // <-- 10.4 mark deleted
                case Kdx_code::MarkDelete: return "kdxlde: mark leaf row deleted"; // <-- 10.5 restore leaf row
                case Kdx_code::UpdateKeyData: return "kdxlup: update keydata in row"; // <-- 10.18 update keydata in row
                case Kdx_code::Purge1:
                case Kdx_code::Purge2:
                    return "kdxlpu: purge leaf row";     // <-- 10.2 insert
                default: return "unknown";
            }
            // 35 <-- 10.35 :: leaf cleanup
        }
        static Result<Kdxlk> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 10.2 #2 Leaf Row Header -- insert
    struct Kdxle {
        uint8_t itl{0};                     // Offset 0
        uint8_t code{0};                    // Offset 1 (0: SINGLE, 0x20: ARRAY)
        uint16_t sno{0};                    // Offset 2
        uint16_t row_size{0};               // Offset 4
        uint16_t key_cnt{0};                // Offset 8 (if ARRAY)
        std::vector<uint16_t> target_slots; // Offset 12 ~ (if ARRAY)

        [[nodiscard]] constexpr bool is_single() const noexcept { return code == 0; }
        [[nodiscard]] constexpr bool is_array()  const noexcept { return code == 0x20; }

        static Result<Kdxle> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 10.18 #2  -- update key data
    struct Kdxlup {
        uint16_t itl{0};       // Transaction Layer Index
        uint16_t sno{0};       // Target Slot Number
        uint16_t row_size{0};  // Row/Update Payload Size
        static Result<Kdxlup> decode(const tcb::span<const char> buf, const bool isLittle);
    };


    // --------------------------------------------------------------------------------
    inline Result<Kdxlup> Kdxlup::decode( const tcb::span<const char> buf, const bool isLittle) {

        if (buf.size() < 6) {
            return err_of(fmt::format("[kdxlup] buf size ({}) < 6", buf.size()));
        }

        return Kdxlup {
            .itl      = decode_At<uint16_t>(buf, isLittle, 0),
            .sno      = decode_At<uint16_t>(buf, isLittle, 2),
            .row_size = decode_At<uint16_t>(buf, isLittle, 4)
        };
    }

    inline Result<Kdxle> Kdxle::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 6) return err_of(fmt::format("[kdxle] buf size ({}) < 6", buf.size()));

        Kdxle h;
        h.itl      = decode_At<uint8_t>(buf, isLittle, 0);
        h.code     = decode_At<uint8_t>(buf, isLittle, 1);
        h.sno      = decode_At<uint16_t>(buf, isLittle, 2);
        h.row_size = decode_At<uint16_t>(buf, isLittle, 4);

        if (h.is_array()) {
            if (buf.size() >= 10) {
                h.key_cnt = decode_At<uint16_t>(buf, isLittle, 8);
            }
            const auto req = 12 + static_cast<size_t>(h.key_cnt) * 2;
            if (buf.size() >= req) {
                h.target_slots.reserve(h.key_cnt);
                for (uint16_t i = 0; i < h.key_cnt; ++i) {
                    h.target_slots.push_back(decode_At<uint16_t>(buf, isLittle, 12 + (i * 2)));
                }
            }
        }
        return h;
    }

    inline Result<Kdxlk> Kdxlk::decode(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < 20) {
            return err_of(fmt::format("[Kdxlk] buf size {} < 20", buf.size()));
        }

        Kdxlk out;
        out.code     = decode_At<uint8_t >(buf, isLittle, 0);
        out.itl      = decode_At<uint8_t >(buf, isLittle, 1);
        out.kdxlkflg = decode_At<uint8_t >(buf, isLittle, 2);
        out.indexid  = decode_At<uint32_t>(buf, isLittle, 4);
        out.block    = decode_At<uint32_t>(buf, isLittle, 8);
        out.sdc      = decode_At<int32_t >(buf, isLittle, 12);

        if (buf.size() >= 24) {
            const auto num = decode_At<uint16_t>(buf, isLittle, 20);
            const auto need = 24 + static_cast<size_t>(num) * 2;
            if (buf.size() >= need) {
                out.key_sizes.reserve(num);
                for (uint16_t j = 0; j < num; ++j) {
                    out.key_sizes.push_back(decode_At<uint16_t>(buf, isLittle, 24 + j * 2));
                }
            } else {
                // In some cases keys-may-not-exist. that's ok. --- ex. 10.35 Index compress operation.
            }
        }
        return out;
    }

    // ----------------------------------------------------------------------------------------------------
    inline std::string to_string(const Kdxlk& a) {
        std::string result = fmt::format("KDXLK: "
                                         "code: 0x{:02x} ({}) itl: {} kdxlkflg: 0x{:02x} "
                                         "indexid: 0x{:x} block: 0x{:08x} sdc: {}",
                                         a.code, a.codeDesc(), a.itl, a.kdxlkflg,
                                         a.indexid, a.block, a.sdc);

        if (!a.key_sizes.empty()) {
            fmt::format_to(std::back_inserter(result), " key_sizes: [{}]", fmt::join(a.key_sizes, ", "));
        }
        return result;
    }

    inline std::string to_string(const Kdxlup& a) {
        return fmt::format("KDXLUP: itl: {} sno: {} row_size: {}", a.itl, a.sno, a.row_size);
    }

    inline std::string to_string(const Kdxle& a) {
        return fmt::format("KDXLE: itl: {} code: {} sno: {} row_size: {} key_cnt: {}, slots#: {}",
            a.itl, a.code, a.sno, a.row_size, a.key_cnt, a.target_slots.size());
    }


}