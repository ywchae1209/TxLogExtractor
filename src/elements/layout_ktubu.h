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

    // --- 1. ~24
    struct Ktubu_base {
        uint32_t objn;     // Object ID
        uint32_t objd;     // Data Object ID
        uint32_t tsn;      // Tablespace ID
        uint32_t prev_dba; // undo-dba or Prev DBA(when over19c)
        uint16_t opc;      // Opcode (Op1 << 8 | Op2)
        uint8_t slt;       // Slot Number
        uint8_t rci;       // Rollback Change Index
        uint16_t flg;      // Flags         -- read when over19c in OLR
        uint16_t wrp;      // Wrap Sequence -- read when over19c in OLR

        bool is_begin_trans() const noexcept { return (flg & Ktub_Flag::BEGIN_TRANS) != 0; }
        bool is_bu_ext() const noexcept { return (flg & Ktub_Flag::BU_EXT) != 0; }

        bool is_mbu_head() const noexcept { return (flg & Ktub_Flag::MBU_HEAD) != 0; }
        bool is_mbu_tail() const noexcept { return (flg & Ktub_Flag::MBU_TAIL) != 0; }
        bool is_mbu_mid() const noexcept { return (flg & Ktub_Flag::MBU_MID) != 0; }
        bool is_regular() const noexcept { return !(is_mbu_head() && is_mbu_tail() && is_mbu_mid()); }

        bool is_lastSplit() const noexcept { return (flg & Ktub_Flag::LAST_SPLIT) != 0; }
        bool is_userUndoDone() const noexcept { return (flg & Ktub_Flag::USER_DONE) != 0; }
        bool is_tempObject() const noexcept { return (flg & Ktub_Flag::TEMP_OBJECT) != 0; }

        static Result<Ktubu_base> decode(tcb::span<const char> buf, bool isLittle);
    };

    inline Result<Ktubu_base> Ktubu_base::decode(tcb::span<const char> buf, bool isLittle) {

        if ( buf.size() < 24)
            return err_of(fmt::format("[Ktubu:base] buf({}) < {}", buf.size(), 24));

        return Ktubu_base {
            .objn     = decode_At<uint32_t>(buf, isLittle, 0),
            .objd     = decode_At<uint32_t>(buf, isLittle, 4),
            .tsn      = decode_At<uint32_t>(buf, isLittle, 8),
            .prev_dba = decode_At<uint32_t>(buf, isLittle, 12),    // previous DBA
            .opc      = static_cast<uint16_t>((decode_At<uint8_t>(buf, isLittle, 16) << 8) |
                                               decode_At<uint8_t>(buf, isLittle, 17)),
            .slt      = decode_At<uint8_t >(buf, isLittle, 18),
            .rci      = decode_At<uint8_t >(buf, isLittle, 19),
            .flg      = decode_At<uint16_t>(buf, isLittle, 20),
            .wrp      = decode_At<uint16_t>(buf, isLittle, 22)
        };
    }

    // --- 2. ~ 28
    struct Ktubu_ext {
        uint16_t flg2;      // Offset 24 ~ 25 : Secondary Flags
        int16_t  buext_idx; // Offset 26 ~ 27 : BuExt Index
    };

    // --- 3. ~76바이트 이상 (Begin Trans) ---
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
    struct Ktubu {
        Ktubu_base          header;
        optional<Ktubu_ext> ext0;             // 28 Bytes Extension
        optional<Ktubl_ext> ext1;             // 76 Bytes Full Extension

        bool is_begin_trans() const noexcept { return (header.flg & Ktub_Flag::BEGIN_TRANS) != 0; }

        bool is_mbu_head() const noexcept { return (header.flg & Ktub_Flag::MBU_HEAD) != 0; }
        bool is_mbu_tail() const noexcept { return (header.flg & Ktub_Flag::MBU_TAIL) != 0; }
        bool is_mbu_mid() const noexcept { return (header.flg & Ktub_Flag::MBU_MID) != 0; }
        bool is_regular() const noexcept { return !(is_mbu_head() && is_mbu_tail() && is_mbu_mid()); }

        bool is_lastSplit() const noexcept { return (header.flg & Ktub_Flag::LAST_SPLIT) != 0; }
        bool is_userUndoDone() const noexcept { return (header.flg & Ktub_Flag::USER_DONE) != 0; }
        bool is_tempObject() const noexcept { return (header.flg & Ktub_Flag::TEMP_OBJECT) != 0; }

        bool has_ubu_ext() const noexcept { return ext0.has_value(); }
        bool has_ubl_ext() const noexcept { return ext1.has_value(); }


        static Result<Ktubu> decode_ktub(tcb::span<const char> buf, bool isLittle, bool hint);

    };

    // --------------------------------------------------------------------------------
    inline Result<Ktubu> Ktubu::decode_ktub(tcb::span<const char> buf, bool isLittle, bool hint) {

        auto base = Ktubu_base::decode(buf, isLittle);
        if (!base) return tl::make_unexpected(base.error());

        const size_t sz = buf.size();

        optional<Ktubu_ext> ext0;
        // ----------------------------------------
        if (sz >= 28 || base->is_bu_ext()) {
            if (enough(buf, 28, "KTUB:ext0")) {
                ext0 = Ktubu_ext{
                    .flg2      = decode_At<uint16_t>(buf, isLittle, 24),
                    .buext_idx = static_cast<int16_t>(decode_At<uint16_t>(buf, isLittle, 26))
                };
            }
        }

        optional<Ktubl_ext> ext1;
        // ----------------------------------------
        const bool is_ktubl = base->is_begin_trans() && hint;
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

        return Ktubu{
            .header   = std::move(*base),
            .ext0     = ext0,
            .ext1     = ext1
        };
    }

    inline std::string to_string(const Ktubu &a) {
        std::string out;
        out.reserve(256);

        // 1. --------------------------------------------
        fmt::format_to(std::back_inserter(out),
            "[{}] objn: {} objd: {} tsn: {} undo(prev_dba): 0x{:08x} opc: {}.{} slt: {} rci: {} flg: 0x{:04x} wrp: {}\n",
            a.has_ubl_ext() ? "KTUBL" : "KTUBU",
            a.header.objn,
            a.header.objd,
            a.header.tsn,
            a.header.prev_dba,
            a.header.opc >> 8,
            a.header.opc & 0xFF,
            a.header.slt,
            a.header.rci,
            a.header.flg,
            a.header.wrp
        );

        // 2. ------------------------------------------
        fmt::format_to(std::back_inserter(out),
            "  └ Flags: [BeginTrans: {} | UserUndoDone: {} | TempObj: {} | TsnUndo: {}]\n",
            a.is_begin_trans() ? "Yes" : "No",
            (a.header.flg & Ktub_Flag::USER_DONE) ? "Yes" : "No",
            (a.header.flg & Ktub_Flag::TEMP_OBJECT)  ? "Yes" : "No",
            (a.header.flg & Ktub_Flag::TS_UNDO)? "Yes" : "No"
        );

        // 3. ------------------------------------------
        if (a.ext0.has_value()) {
            const auto& e0 = *a.ext0;
            fmt::format_to(std::back_inserter(out),
                "  └ [Ext0] buext_idx: {} flg2: 0x{:04x}\n",
                e0.buext_idx,
                e0.flg2
            );
        }

        // 4. ------------------------------------------
        if (a.ext1.has_value()) {
            const auto& e1 = *a.ext1;
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