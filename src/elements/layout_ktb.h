#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>
#include "../coral_decode.h"
#include "../coral_result.h"
#include "layout_common.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

//https://github.com/bersler/OpenLogReplicator/blob/6bc92bc1b89255fbc491e3080cb12a4c1dd8e832/src/parser/OpCode.h#L136
namespace ora {

    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::vector, std::variant, std::monostate;

    // --- KTB Operation Constants (from OpenLogReplicator) ---
    namespace KtbOpCode {
        constexpr uint8_t MASK           = 0x0F;
        constexpr uint8_t F              = 0x01; // Flush/Tx
        constexpr uint8_t C              = 0x02; // Commit
        constexpr uint8_t Z              = 0x03; // Zero/Reset
        constexpr uint8_t L              = 0x04; // Lock / ITL Change
        constexpr uint8_t R              = 0x05; // ITL Table Redo
        constexpr uint8_t N              = 0x06; // None
        constexpr uint8_t BLOCK_CLEANOUT = 0x10; // Block Cleanout Bit
    }

    constexpr char ktb_op_code(uint8_t op_sub) noexcept {
        switch (op_sub & KtbOpCode::MASK) {
            case KtbOpCode::F: return 'F';
            case KtbOpCode::C: return 'C';
            case KtbOpCode::Z: return 'Z';
            case KtbOpCode::L: return 'L';
            case KtbOpCode::R: return 'R';
            case KtbOpCode::N: return 'N';
            default:           return '?';
        }
    }

    // --- structs ---
    struct Ktb_OpC {
        Ktb_uba7 uba;
    };

    struct Ktb_OpL {
        Ktb_xid8  xid;
        Ktb_uba7  uba;
        uint8_t   lkc;
        uint8_t   flag;
        Ktb_scn6  scn;

        [[nodiscard]] std::string flag_string() const {

            char flagStr[5]{"----"};
            if ((flag & 0x10) != 0) flagStr[3] = 'T';
            if ((flag & 0x20) != 0) flagStr[2] = 'U';
            if ((flag & 0x40) != 0) flagStr[1] = 'B';
            if ((flag & 0x80) != 0) flagStr[0] = 'C';

            return flagStr;
        }
    };

    struct Ktb_OpR {
        int16_t             itc;
        vector<Ktb_ItlEntry> entries;
    };

    struct Ktb_OpF {
        Ktb_xid8 xid;
        Ktb_uba7 uba;
    };

    using Ktb_OpData = variant<
        monostate, // Z, N
        Ktb_OpC,
        Ktb_OpL,
        Ktb_OpR,
        Ktb_OpF
    >;

    struct Ktb_CleanoutEntry {
        uint8_t  itli;
        uint8_t  flg;
        Ktb_scn6 scn;
    };

    struct Ktb_Cleanout {
        uint64_t   scn;
        uint8_t    opt;
        uint8_t    ver;
        uint8_t    entries_cnt;
        vector<Ktb_CleanoutEntry> entries;
    };

    /// has ktb_op ::
    struct KtbVector {
        uint8_t    ktb_op;        //
        uint8_t    flg;           //
        uint8_t    ver;           // flg & 0x03
        bool       post_11g;      // (flg & 0x04) != 0
        char       op_code;       // 'C', 'Z', 'L', 'R', 'N', 'F'
        Ktb_OpData op_data;       // Sub-Op 상세 데이터
        optional<Ktb_Cleanout> cleanout;      // (ktb_op & 0x10) 인 경우
    };

    inline uint64_t decode_ktb_scn8(tcb::span<const char> buf, bool isLittle, size_t offset) {
        return (static_cast<uint64_t>(
            decode_At<uint32_t>(buf, isLittle, offset)) << 32) |
            decode_At<uint32_t>(buf, isLittle, offset + 4);  // todo :: check
    }

    // --------------------------------------------------------------------------------

    // 'C' (Commit, 0x02)
    inline Result<Ktb_OpData> decode_ktb_op_c(tcb::span<const char> buf, bool isLittle, size_t sp) {
        if (auto check = enough(buf, sp + 8, "KtbRedo:C"); !check) return tl::make_unexpected(check.error());

        return Ktb_OpData{
            Ktb_OpC{
                .uba = decode_ktb_uba7(buf, isLittle, sp)
            }
        };
    }

    // 'L' (Lock / ITL Change, 0x04)
    inline Result<Ktb_OpData> decode_ktb_op_l(tcb::span<const char> buf, bool isLittle, size_t sp) {
        if (auto check = enough(buf, sp + 24, "KtbRedo:L"); !check) return tl::make_unexpected(check.error());

        const auto xid  = decode_ktb_xid8(    buf, isLittle, sp);
        const auto uba  = decode_ktb_uba7(    buf, isLittle, sp + 8);
        const auto l_f  = decode_At<uint16_t>(buf, isLittle, sp + 16);
        const auto scn  = decode_ktb_scn6(    buf, isLittle, sp + 18);

        const auto lkc  = static_cast<uint8_t>(l_f >> 8);
        const auto flag = static_cast<uint8_t>(l_f & 0xFF);

        return Ktb_OpData{
            Ktb_OpL{
                .xid  = xid ,
                .uba  = uba ,
                .lkc  = lkc ,
                .flag = flag,
                .scn  = scn,
            }
        };
    }

    // 'R' (ITL Table Redo, 0x05)
    inline Result<Ktb_OpData> decode_ktb_op_r(tcb::span<const char> buf, bool isLittle, size_t sp) {
        if (auto check = enough(buf, sp + 4, "KtbRedo:R Header"); !check) return tl::make_unexpected(check.error());

        const auto itc = decode_At<int16_t>(buf, isLittle, sp + 2);
        const auto valid_itc = std::max<int16_t>(itc, 0); // (-) exists ?

        const auto req = sp + 12 + (valid_itc * 24);
        if (auto check = enough(buf, req, "KtbRedo:R Total"); !check) return tl::make_unexpected(check.error());

        vector<Ktb_ItlEntry> entries;
        entries.reserve(valid_itc);

        for (auto i = 0; i < valid_itc; ++i) {
            const size_t off = sp + 12 + (i * 24);
            entries.push_back( decode_ktb_itlEntry24(buf, isLittle, off) );
        }

        return Ktb_OpData{
            Ktb_OpR{
                .itc = itc,
                .entries = std::move(entries)
            }
        };
    }

    // 'F' (Flush/Tx, 0x01)
    inline Result<Ktb_OpData> decode_ktb_op_f(tcb::span<const char> buf, bool isLittle, size_t sp) {
        if (auto check = enough(buf, sp + 16, "KtbRedo:F"); !check) return tl::make_unexpected(check.error());

        return Ktb_OpData{ Ktb_OpF{
                .xid = decode_ktb_xid8(buf, isLittle, sp),
                .uba = decode_ktb_uba7(buf, isLittle, sp + 8)
            }
        };
    }

    // --------------------------------------------------------------------------------
    // Cleanout :: offset = (sp + 36)
    inline Result<Ktb_Cleanout> decode_ktb_cleanout(tcb::span<const char> buf, bool isLittle, size_t offset) {

        if (auto check = enough(buf, offset + 12, "KtbCleanout:H"); !check) {
            return tl::make_unexpected(check.error());
        }

        const auto opt = decode_At<uint8_t>(buf, isLittle, offset + 0);  // sp + 36
        const auto cnt = decode_At<uint8_t>(buf, isLittle, offset + 1);  // sp + 37
        const auto ver = decode_At<uint8_t>(buf, isLittle, offset + 2);  // sp + 38
        const auto scn = decode_ktb_scn8(   buf, isLittle, offset + 4);  // sp + 40

        const size_t req = offset + 12 + (cnt * 8U);
        if (auto check = enough(buf, req, "KtbCleanout:Total"); !check) {
            return tl::make_unexpected(check.error());
        }

        vector<Ktb_CleanoutEntry> entries;
        entries.reserve(cnt);

        for (uint8_t j = 0; j < cnt; ++j) {
            const size_t c_off = offset + 12 + (j * 8); // sp + 48 + (j * 8)
            entries.push_back(Ktb_CleanoutEntry{
                .itli = decode_At<uint8_t>(buf, isLittle, c_off),
                .flg  = decode_At<uint8_t>(buf, isLittle, c_off + 1),
                .scn  = decode_ktb_scn6(   buf, isLittle, c_off + 2)
            });
        }

        return Ktb_Cleanout{
            .scn         = scn,
            .opt         = opt,
            .ver         = ver,
            .entries_cnt = cnt,
            .entries     = std::move(entries)
        };
    }

    [[nodiscard]] inline Result<KtbVector> decode_ktb(tcb::span<const char> buf, bool isLittle) {
        if (auto check = enough(buf, 8, "Ktb:min"); !check) return tl::make_unexpected(check.error());

        const auto op0  = decode_At<uint8_t>(buf, isLittle, 0);
        const auto flg = decode_At<uint8_t>(buf, isLittle, 1);

        const auto sp  = ((flg & 0x08) == 0) ? 4 : 8;
        const auto op_sub = op0 & KtbOpCode::MASK;

        // --------------------------------------------------------------------------------
        Ktb_OpData op_data = monostate{};
        switch (op_sub) {
            case KtbOpCode::C: {
                if (auto d = decode_ktb_op_c(buf, isLittle, sp)) op_data = std::move(*d);
                else return tl::make_unexpected(d.error());

                break;
            }
            case KtbOpCode::L: {
                auto d = decode_ktb_op_l(buf, isLittle, sp);
                if (!d) return tl::make_unexpected(d.error());
                op_data = std::move(*d);
                break;
            }
            case KtbOpCode::R: {
                auto d = decode_ktb_op_r(buf, isLittle, sp);
                if (!d) return tl::make_unexpected(d.error());
                op_data = std::move(*d);
                break;
            }
            case KtbOpCode::F: {
                auto d = decode_ktb_op_f(buf, isLittle, sp);
                if (!d) return tl::make_unexpected(d.error());
                op_data = std::move(*d);
                break;
            }
            case KtbOpCode::Z:
            case KtbOpCode::N:
            default:
                break;
        }

        // Block Cleanout Record (0x10 비트)
        optional<Ktb_Cleanout> cleanout;
        if ((op0 & KtbOpCode::BLOCK_CLEANOUT) != 0) {
            auto co = decode_ktb_cleanout(buf, isLittle, sp + 36);
            if (!co) return tl::make_unexpected(co.error());
            cleanout = std::move(*co);
        }

        return KtbVector{
            .ktb_op   = op0,
            .flg      = flg,
            .ver      = static_cast<uint8_t>(flg & 0x03),
            .post_11g = (flg & 0x04) != 0,
            .op_code  = ktb_op_code(op0),
            .op_data  = std::move(op_data),
            .cleanout = std::move(cleanout)
        };
    }

    static std::string to_string(KtbVector const &a) {
        return "";
    }
}