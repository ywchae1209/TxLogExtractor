#include "../coral_show.h"
#include "../blocks/Block.h"
#include "Record.h"
#include "layout_RecordHead.h"

namespace ora {
    RecordHead RecordHead_of(const tcb::span<const char>& raw, const bool isLittle) {

        constexpr auto sz0 = sizeof(Base_lo);
        constexpr auto sz1 = sizeof(Lwn_lo);

        if (raw.size() < sz0) {
            throw std::out_of_range("[RecordHead_of] Buffer size < 24 byte.");
        }

        auto base = decode_record_head_base(raw, isLittle);

        if (!dependBit_on(base.vld))
            return RecordHead{
                .size = sz0,
                .base = base,
                .lwn  = std::nullopt};

        if (raw.size() < sz0 + sz1)
            throw std::out_of_range("[RecordHead_of] Buffer size < 64 byte.");

        auto ext = decode_record_head_lwn(raw.subspan(sz0), isLittle);

        return RecordHead{
            .size = sz0 + sz1,
            .base = base,
            .lwn = ext };
    }

    // --------------------------------------------------------------------------------
    Record Record_of(
        const RBA& rba,
        RecordPayload& bytes, // const std::vector<char>& bytes,
        const RecordBound& bound,
        const BlockCtx& ctx ) {

        auto end_block = rba.block_no + bound.next_blocks;
        auto end_offset = bound.next_offset;
        auto header = RecordHead_of(bytes.asVector(), ctx.isLittle);

        return Record{
            .rba = rba,
            .end_block = end_block,
            .end_offset = end_offset,
            .raw = std::move(bytes),
            .header = header,
            .over12c = ctx.over12c,
            .isLittle = ctx.isLittle,
            .isVoid = bound.boundInfo.vld == 0
        };
    }

    // --------------------------------------------------------------------------------
    using coral::toHex;
    std::string to_string(const RecordHead &h) {

        const auto &b = h.base;
        std::string s = fmt::format(
            "LEN:{:<6} "
            "SCN: 0x{:04x}.{:08x}:{:03} CONID: {} "
            "-- {:3} -- "
            "VLD:{} {}",
            b.len,
            b.scn_wrap, b.scn_base, b.sub_scn, b.container_id,
            h.size,
            toHex(b.vld), vld_string(b.vld)
        );

        if (h.lwn) {
            const auto &l = *h.lwn;
            s += fmt::format(
                "  -- [LWN] nst:{} next:{:<3} len:{:<3} start_scn:{} next_scn:{} Epoch:{}",
                l.lwn_nst,
                l.lwn_next,
                l.lwn_length,
                toHex0(l.lwn_start_scn),
                toHex0(l.lwn_next_scn),
                l.epoch
            );
        }

        return s;
    }

    // --------------------------------------------------------------------------------
    inline void show(const Change& c, uint8_t showMode, std::ostream& os) {

        const auto dump = (showMode & 2) == 2;
        const auto info = (showMode & 1) == 1;
        if (showMode == 0) return;

        if (info) fmt::println( "{}", to_string(c.change_head));
        if (dump) coral::show_HexDump(c.change_head.span);
        fmt::println( "  ** LV : [{}]", fmt::join(c.length_vector.sizes, ", ") );
        if (dump) show(c.length_vector, dump);
    }

    void show(Record& r, uint8_t showMode, std::ostream &os) {
        fmt::println(os,
            "R {}{} ~{:>7}.@{:<3}{} | "
            "{}",
            coral::key_color, to_string(r.rba), r.end_block, r.end_offset, coral::reset_color,
            to_string(r.header)
        );

        if (showMode != 0) {
            const auto ret = r.changes();
            if (!ret) {
                fmt::println(std::cerr, "  * Change exceptions: {}{}{}", coral::error_color, ret.error(), coral::reset_color);
                fmt::println(os, "  * Change exceptions: {}{}{}", coral::error_color, ret.error(), coral::reset_color);
                return;
            }
            const auto& cs = *ret;
            fmt::println(os, "  * Change Count == {}", cs.size());
            for (const auto& c : cs) {
                show(c, showMode, os);
            }
        }
    }
}