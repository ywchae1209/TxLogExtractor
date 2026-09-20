#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_result.h"
#include "layout_common.h"

// https://github.com/bersler/OpenLogReplicator/blob/6bc92bc1b89255fbc491e3080cb12a4c1dd8e832/src/parser/OpCode.h#L1783
namespace ora {

    using coral::decode_at, coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::variant, std::monostate;

    // --- (from: OpenLogReplicator) ---
    namespace Ktub_Flag {
        constexpr uint16_t MBU_HEAD       = 0x0001;
        constexpr uint16_t MBU_TAIL       = 0x0002;
        constexpr uint16_t LAST_SPLIT     = 0x0004;
        constexpr uint16_t BEGIN_TRANS    = 0x0008;
        constexpr uint16_t USER_DONE      = 0x0010;
        constexpr uint16_t TEMP_OBJECT    = 0x0020;
        constexpr uint16_t USER_ONLY      = 0x0040;
        constexpr uint16_t TS_UNDO        = 0x0080;
        constexpr uint16_t MBU_MID        = 0x0100;
        constexpr uint16_t BU_EXT         = 0x0800;
    }

    // --- 1. 24
    struct Ktub_base {
        uint32_t objn; // Offset 0 ~ 3   : Object ID
        uint32_t objd; // Offset 4 ~ 7   : Data Object ID
        uint32_t tsn;  // Offset 8 ~ 11  : Tablespace ID
        uint32_t undo; // Offset 12 ~ 15 : Undo Data Block Address / Prev DBA
        uint16_t opc;  // Offset 16 ~ 17 : Opcode (Op1 << 8 | Op2)
        uint8_t slt;   // Offset 18      : Slot Number
        uint8_t rci;   // Offset 19      : Rollback Change Index
        uint16_t flg;  // Offset 20 ~ 21 : Flags
        uint16_t wrp;  // Offset 22 ~ 23 : Wrap Sequence
    };

    // --- 2. ~ 28
    struct Ktubu_ext {
        uint16_t flg2;      // Offset 24 ~ 25 : Secondary Flags
        int16_t  buext_idx; // Offset 26 ~ 27 : BuExt Index
    };

    // --- 3. ~76바이트 이상 KTUBL 전용 상세 확장 (Begin Trans) ---
    struct Ktubl_ext {
        Ktb_uba7 prev_ctl_uba;         // Offset 28 ~ 34 (7 Bytes: UBA)
        uint64_t prev_ctl_max_cmt_scn; // Offset 36 ~ 43 (8 Bytes: SCN)
        uint64_t prev_tx_cmt_scn;      // Offset 44 ~ 51 (8 Bytes: SCN)
        uint64_t tx_start_scn;         // Offset 56 ~ 63 (8 Bytes: SCN)
        uint32_t prev_brb;             // Offset 64 ~ 67 : Prev Block Rollback Block
        uint32_t prev_bcl;             // Offset 68 ~ 71 : Prev Block Cleanout
        uint32_t logon_user;           // Offset 72 ~ 75 : Logon User ID
    };

    // ---
    /// Ktubu | Ktubl
    struct Ktub {
        Ktub_base           header;
        optional<Ktubu_ext> ext0;             // 28 Bytes Extension
        optional<Ktubl_ext> ext1;             // 76 Bytes Full Extension

        [[nodiscard]] bool is_begin_trans() const noexcept { return (header.flg & Ktub_Flag::BEGIN_TRANS) != 0; }

        [[nodiscard]] bool is_mbu_head() const noexcept { return (header.flg & Ktub_Flag::MBU_HEAD) != 0; }
        [[nodiscard]] bool is_mbu_tail() const noexcept { return (header.flg & Ktub_Flag::MBU_TAIL) != 0; }
        [[nodiscard]] bool is_mbu_mid() const noexcept { return (header.flg & Ktub_Flag::MBU_MID) != 0; }
        [[nodiscard]] bool is_regular() const noexcept { return !(is_mbu_head() && is_mbu_tail() && is_mbu_mid()); }

        [[nodiscard]] bool is_lastSplit() const noexcept { return (header.flg & Ktub_Flag::LAST_SPLIT) != 0; }
        [[nodiscard]] bool is_userUndoDone() const noexcept { return (header.flg & Ktub_Flag::USER_DONE) != 0; }
        [[nodiscard]] bool is_tempObject() const noexcept { return (header.flg & Ktub_Flag::TEMP_OBJECT) != 0; }


        [[nodiscard]] bool has_bu_ext() const noexcept { return ext0.has_value(); }
        [[nodiscard]] bool has_bl_ext() const noexcept { return ext1.has_value(); }
    };

    // --------------------------------------------------------------------------------
    inline Result<Ktub> decode_ktub(tcb::span<const char> buf, bool isLittle, bool hint) {

        if (auto check = enough(buf, 24, "KTUB:Header"); !check) { return tl::make_unexpected(check.error()); }

        // ----------------------------------------
        const Ktub_base header{
            .objn  = decode_At<uint32_t>(buf, isLittle, 0),
            .objd  = decode_At<uint32_t>(buf, isLittle, 4),
            .tsn   = decode_At<uint32_t>(buf, isLittle, 8),
            .undo  = decode_At<uint32_t>(buf, isLittle, 12),    // previous DBA
            .opc   = static_cast<uint16_t>((decode_At<uint8_t>(buf, isLittle, 16) << 8) | decode_At<uint8_t>(buf, isLittle, 17)),
            .slt   = decode_At<uint8_t>(buf, isLittle, 18),
            .rci   = decode_At<uint8_t>(buf, isLittle, 19),
            .flg   = decode_At<uint16_t>(buf, isLittle, 20),
            .wrp   = decode_At<uint16_t>(buf, isLittle, 22)
        };

        const bool is_ktubl = (header.flg & Ktub_Flag::BEGIN_TRANS) != 0 && hint;
        const size_t sz = buf.size();

        optional<Ktubu_ext> ext0;
        optional<Ktubl_ext> ext1;
        // ----------------------------------------
        if (sz >= 28 || (header.flg & Ktub_Flag::BU_EXT) != 0) {
            if (enough(buf, 28, "KTUB:ext0")) {
                ext0 = Ktubu_ext{
                    .flg2      = decode_At<uint16_t>(buf, isLittle, 24),
                    .buext_idx = static_cast<int16_t>(decode_At<uint16_t>(buf, isLittle, 26))
                };
            }
        }
        // ----------------------------------------
        if (is_ktubl && sz >= 76) {
            ext1 = Ktubl_ext{
                .prev_ctl_uba         = decode_ktb_uba7(buf, isLittle, 28),
                .prev_ctl_max_cmt_scn = decode_ktb_scn8(buf, isLittle, 36),
                .prev_tx_cmt_scn      = decode_ktb_scn8(buf, isLittle, 44),
                .tx_start_scn         = decode_ktb_scn8(buf, isLittle, 56),
                .prev_brb             = decode_At<uint32_t>(buf, isLittle, 64),
                .prev_bcl             = decode_At<uint32_t>(buf, isLittle, 68),
                .logon_user           = decode_At<uint32_t>(buf, isLittle, 72)
            };
        }

        return Ktub{
            .header   = header,
            .ext0     = ext0,
            .ext1     = ext1
        };
    }

    inline std::string to_string(const Ktub &ktub) {
        std::string out;
        out.reserve(256);

        // 1. --------------------------------------------
        fmt::format_to(std::back_inserter(out),
            "[{}] objn: {} objd: {} tsn: {} undo(prev_dba): 0x{:08x} opc: {}.{} slt: {} rci: {} flg: 0x{:04x} wrp: {}\n",
            ktub.has_bl_ext ? "KTUBL" : "KTUBU",
            ktub.header.objn,
            ktub.header.objd,
            ktub.header.tsn,
            ktub.header.undo,
            ktub.header.opc >> 8,
            ktub.header.opc & 0xFF,
            ktub.header.slt,
            ktub.header.rci,
            ktub.header.flg,
            ktub.header.wrp
        );

        // 2. ------------------------------------------
        fmt::format_to(std::back_inserter(out),
            "  └ Flags: [BeginTrans: {} | UserUndoDone: {} | TempObj: {} | TsnUndo: {}]\n",
            ktub.is_begin_trans() ? "Yes" : "No",
            (ktub.header.flg & Ktub_Flag::USER_DONE) ? "Yes" : "No",
            (ktub.header.flg & Ktub_Flag::TEMP_OBJECT)  ? "Yes" : "No",
            (ktub.header.flg & Ktub_Flag::TS_UNDO)? "Yes" : "No"
        );

        // 3. ------------------------------------------
        if (ktub.ext0.has_value()) {
            const auto& e0 = *ktub.ext0;
            fmt::format_to(std::back_inserter(out),
                "  └ [Ext0] buext_idx: {} flg2: 0x{:04x}\n",
                e0.buext_idx,
                e0.flg2
            );
        }

        // 4. ------------------------------------------
        if (ktub.ext1.has_value()) {
            const auto& e1 = *ktub.ext1;
            fmt::format_to(std::back_inserter(out),
                "  └ [Ext1] prev_ctl_uba: [dba:0x{:08x} sqn:{} rec:{}]\n"
                "           prev_ctl_max_cmt_scn: {}\n"
                "           prev_tx_cmt_scn: {}\n"
                "           tx_start_scn: {}\n"
                "           prev_brb: 0x{:08x} prev_bcl: 0x{:08x} logon_user: {}\n",
                e1.prev_ctl_uba.dba, e1.prev_ctl_uba.sqn, e1.prev_ctl_uba.rec,
                e1.prev_ctl_max_cmt_scn,
                e1.prev_tx_cmt_scn,
                e1.tx_start_scn,
                e1.prev_brb,
                e1.prev_bcl,
                e1.logon_user
            );
        }
        return out;
    }





}