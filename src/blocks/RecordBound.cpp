#include "Block.h"

#include "BlockSource.h"
#include "layout_BlockHead.h"
#include "../coral_show.h"

#include "range/v3/view/transform.hpp"

namespace ora {
    using fmt::format;
    using std::cerr;

    // from limited source.
    enum VldFlags : uint8_t {
        KCR_void = 0x00, // Invalid
        KCR_valid = 0x01, // Valid record
        KCR_commit = 0x02, // Commit SCN
        KCR_depend = 0x04, // Dependent SCN
        KCR_newMark = 0x08, // New SCN mark
        KCR_oldMark = 0x10, // Old SCN mark
        KCR_gno = 0x20, // Group/Thread
        KCR_timeWrap = 0x40, // Timewarp
        KCR_raw = 0x80 // Raw Record
    };

    [[nodiscard]] bool dependBit_on(uint8_t vld) {
        return (vld & KCR_depend) == KCR_depend;
    }

    [[nodiscard]] std::optional<bool> choose_new_vld(uint8_t vld0, uint8_t vld1) {
        if (vld0 == vld1) return std::nullopt;

        if (vld0 != 1 && vld1 == 1) return true;
        if (vld0 == 1 && vld1 != 1) return false;

        constexpr uint8_t MASK = KCR_valid | KCR_commit | KCR_depend;
        const bool vld0_has = (vld0 & MASK != 0);
        const bool vld1_has = (vld1 & MASK != 0);

        if (!vld0_has && vld1_has) return true;
        if (vld0_has && !vld1_has) return false;

        return vld0 > vld1; // lower VLD maybe more correct.
    }

    [[nodiscard]] std::string vld_string(uint8_t vld) {
        // 0x00 == invalid |  Zero Padding
        if (vld == KCR_void) {
            return "void";
        }

        std::string result;
        result.reserve(64);

        auto append_flag = [&result](const char *flag_name) {
            if (!result.empty()) {
                result += '|';
            }
            result += flag_name;
        };

        if (vld & KCR_valid) append_flag("valid");
        if (vld & KCR_commit) append_flag("commit");
        if (vld & KCR_depend) append_flag("depend");
        if (vld & KCR_newMark) append_flag("nmark");
        if (vld & KCR_oldMark) append_flag("omark");
        if (vld & KCR_gno) append_flag("gno");
        if (vld & KCR_timeWrap) append_flag("timewrap");
        if (vld & KCR_raw) append_flag("raw");

        return result;
    }

    bool bad_record_len(uint32_t l) noexcept {
        constexpr uint32_t MIN_RECORD = 24;
        constexpr uint64_t MAX_RECORD_LEN = 16 * 1024 * 1024; // 16 MB

        const bool ok = // (l & 3) == 0 && // 4의 배수
                        MIN_RECORD <= l && l <= MAX_RECORD_LEN;

        return !ok;
    }

    inline bool bad_vld(const uint8_t v) noexcept {

        if (v == KCR_void )
            return false; // maybe padding.

        // 하위 5bit : 0x1F : 하나는 setting되어 있어야 함.
        constexpr uint8_t VLD_CORE_MASK = KCR_valid | KCR_commit | KCR_depend | KCR_newMark | KCR_oldMark;

        // new-mark(0x08)와 old-mark(0x10) 동시 포함 불가
        constexpr uint8_t VLD_MARK_CONFLICT_MASK = KCR_newMark | KCR_oldMark;

        const bool bad =
                // v == KCR_void || // must not void(0)
                (v & VLD_CORE_MASK) == 0 || // must contain one
                (v & VLD_MARK_CONFLICT_MASK) == VLD_MARK_CONFLICT_MASK ; // must not conflict new/old

                // raw must contain valid -- found invalid case :: archive_dump_2/ORASSO_1_7839_1115373628.arc
                // ((v & KCR_raw) != 0 && (v & KCR_valid) == 0);

        return bad;
    }

    // --------------------------------------------------------------------------------
    // un-official
    namespace FooFlags {
        constexpr uint8_t SUB_COMMIT = 0x01; // Commit SCN Marker
        constexpr uint8_t SUB_ROLLBACK = 0x02; // Rollback Marker
        constexpr uint8_t SUB_CKPT = 0x04; // Checkpoint
        constexpr uint8_t SUB_HBKUP = 0x08; // Hot Backup
        constexpr uint8_t SUB_DIRECT = 0x10; // Direct-Path-Write
        constexpr uint8_t SUB_RECOVERY = 0x20; // Media Recovery Control
        constexpr uint8_t SUB_MULTI_TRAN = 0x40; // Distributed / Multi-Transaction
        constexpr uint8_t SUB_SPECIAL = 0x80; // Special System Record
    }

    [[nodiscard]] std::string foo_string(uint8_t foo) {
        if (foo == 0x00) {
            return "normal";
        }

        std::string result;
        result.reserve(64);

        auto append_flag = [&result](const char *flag_name) {
            if (!result.empty()) {
                result += '|';
            }
            result += flag_name;
        };

        if (foo & FooFlags::SUB_COMMIT) append_flag("commit");
        if (foo & FooFlags::SUB_ROLLBACK) append_flag("rollback");
        if (foo & FooFlags::SUB_CKPT) append_flag("ckpt");
        if (foo & FooFlags::SUB_HBKUP) append_flag("hbkup");
        if (foo & FooFlags::SUB_DIRECT) append_flag("direct");
        if (foo & FooFlags::SUB_RECOVERY) append_flag("recovery");
        if (foo & FooFlags::SUB_MULTI_TRAN) append_flag("multi_tran");
        if (foo & FooFlags::SUB_SPECIAL) append_flag("special");

        return result;
    }

    // --------------------------------------------------------------------------------
    static RecordBound Bound_of(const BoundInfo &bi,
                                const uint32_t offset,
                                const uint32_t block_sz) {

        const auto record_len = bi.len;

        const auto room = block_sz - BLOCK_HEAD;
        const auto room0 = block_sz - offset; // current room

        const int32_t needs = record_len - room0;

        if (needs < 0)  return RecordBound{record_len, 0, static_cast<uint16_t>(offset + record_len), bi};
        if (needs == 0) return RecordBound{record_len, 1, BLOCK_HEAD, bi};

        const uint32_t next_block = (needs / room) + 1;
        const uint16_t remain = needs % room;
        const uint16_t next_offset = (remain == 0) ? BLOCK_HEAD : static_cast<uint16_t>(BLOCK_HEAD + remain);

        return RecordBound{record_len, next_block, next_offset, bi};
    }

    inline uint64_t align_up4(const uint64_t size) noexcept {
        return (size + 3) & ~static_cast<uint64_t>(3);
    }


    inline bool in_range (uint64_t n, uint64_t low, uint64_t top) noexcept {
        return low <= n && n <= top;
    }

    inline bool out_range (uint64_t n, uint64_t low, uint64_t top) noexcept {
        return !in_range(n, low, top);
    }

    // --------------------------------------------------------------------------------
    static std::optional<BoundInfo> read_BoundInfo0(const tcb::span<const char>& view,
                                                    const uint32_t block_no,
                                                    const size_t offset,
                                                    const BlockCtx &ctx,
                                                    const bool showReason ) {
        using coral::get_at;

        if (offset < 16) return std::nullopt;

        //// caveat :: assumption must contain 12 bytes  ---  this is a strong assumption.
        if (offset > (view.size() - 12)) {
            if (showReason)
                fmt::println( "--#{}.@{:03} bad offset {} > {} ",
                              block_no, offset, offset, view.size() - 12) ;
            return std::nullopt;
        }

        const auto len  = get_at<uint32_t>(view, offset, ctx.isLittle);
        const auto vld  = get_at< uint8_t>(view, offset + 4, ctx.isLittle);
        const auto foo  = get_at< uint8_t>(view, offset + 5, ctx.isLittle);
        const auto wrap = get_at<uint16_t>(view, offset + 6, ctx.isLittle);
        const auto base = get_at<uint32_t>(view, offset + 8, ctx.isLittle);

        const auto cur = scn_to64(wrap, base);

        auto log_out = [&](std::string s) {
            if (showReason)
                fmt::println(format("--#{}.@{:03} bad {} "
                                    "0x{:04x}.{:08x} {}:{} "
                                    "ctx: 0x{:04x}.{:08x} 0x{:04x}.{:08x}",
                                    block_no, offset, s,
                                    wrap, base, len, vld,
                                    ctx.low_scn.wrap, ctx.low_scn.base,
                                    ctx.nxt_scn.wrap, ctx.nxt_scn.base ) );
            return std::nullopt;
        };

        constexpr uint32_t MIN_RECORD = 24;
        constexpr uint64_t MAX_RECORD_LEN = 16 * 1024 * 1024; // 16 MB
        const bool no_len = len == 0 || (len & 3) != 0 || len < MIN_RECORD;
        if (no_len) return std::nullopt; // surely invalid

        if (len > MAX_RECORD_LEN) return log_out("LEN");
        if (out_range(cur, ctx.low(), ctx.nxt())) return log_out("SCN");
        if (bad_vld(vld)) return log_out("VLD");

        return std::make_optional(BoundInfo{len, SCN{base, wrap}, vld, foo});
    }

    // --------------------------------------------------------------------------------
    std::vector<RecordBound> bound_candidates(const tcb::span<const char>& view,
                                               const uint16_t start_offset,
                                               const uint32_t block_no,
                                               const BlockCtx &ctx,
                                               const bool showReason) {
        std::vector<RecordBound> result;

        auto offset = start_offset;
        auto bi0 = read_BoundInfo0(view, block_no, offset, ctx, showReason);

        int limit = 0;
        while (bi0 && limit < 32) {
            if (bi0->len == 0) break;

            auto bound = Bound_of(*bi0, offset, ctx.block_sz);
            result.push_back(bound);
            limit++;

            if (bound.next_blocks > 0) break;

            offset = bound.next_offset;
            bi0 = read_BoundInfo0(view, block_no, offset, ctx, showReason);
        }

        return result;
    }

    std::vector<RecordBound> fairBounds(const Block& block,
                                        const std::optional<LwnCtx> &lwn,
                                        const bool showReason) {
        const auto& bs = block.bounds;
        const auto lsn = block.log_seq_no();
        const auto bsn = block.block_no();

        std::vector<RecordBound> out;
        out.reserve(bs.size());

        auto off = block.offset();
        for ( const auto& b : bs ) {

            const auto rba = RBA{ lsn, bsn, off };
            if (fairBound(b, lwn, showReason, rba))
                out.push_back(b);

            off = b.next_offset;
        }
        return out;
    }

    bool fairBound(const RecordBound &b, const std::optional<LwnCtx> &lwn, const bool showReason, const RBA &rba) {

        if (!lwn) return true;

        const auto& bi = b.boundInfo;

        const auto dependOn = bi.dependOn();
        const auto cur = bi.scn64();
        const auto low = lwn->low();

        if (cur < low) return false;  // surely no.

        const auto top = lwn->nxt();
        const auto dif0 = static_cast<int64_t>(cur - low);
        const auto diff = static_cast<int64_t>(cur - top);

        auto log_out = [&](std::string_view s) {
            if (!showReason) return;
            fmt::println(
                format("--#{}.@{:03} BY {} +low {} +high {}  "
                       "0x{:04x}.{:08x} {}:{} "
                       "lwn: 0x{:04x}.{:08x} 0x{:04x}.{:08x}",
                       rba.block_no, rba.offset, s, dif0, diff,
                       bi.scn.wrap, bi.scn.base, bi.len, bi.vld,
                       lwn->l_low.wrap, lwn->l_low.base, lwn->l_nxt.wrap, lwn->l_nxt.base));
        };

        if (dependOn) {
            if ( diff <= 1) return true;    // surely ok.
            log_out(diff > 1024 ? "ok++?" : "ok+ ?");
            return true;
        }
        if ( diff <= 0) return true;
        log_out(diff >= 2048
                    ? "no---"
                    : diff > 100 ? "ok--?" : "ok- ?");

        return diff < 2048;
    }
    // --------------------------------------------------------------------------------
}
