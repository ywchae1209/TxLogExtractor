#include "../blocks/Block.h"
#include "Record.h"
#include "layout_RecordHead.h"

namespace ora {
    RecordHead RecordHead_of(const tcb::span<const char>& raw, const bool isLittle) {

        constexpr auto size0 = sizeof(Base_lo);
        constexpr auto size1 = sizeof(Lwn_lo);

        if (raw.size() < size0) {
            throw std::out_of_range("[RecordHead_of] Buffer size < 24 byte.");
        }

        auto base = [&]() -> RecordHead_Base {
            Base_lo raw_base{};
            std::memcpy(&raw_base, raw.data(), size0);
            return decode(raw_base, isLittle);
        }();

        //// variable length Header :: vld ~ dependBit
        if (!dependBit_on(base.vld) )
            return RecordHead{ size0, base, std::nullopt};

        if (raw.size() < size0 + size1) {
            throw std::out_of_range("[RecordHead_of] Buffer size < 64 byte.");
        }

        auto ext = [&]() -> auto {
            Lwn_lo o{};
            std::memcpy(&o, raw.data() + size0, size1);
            return decode(o, isLittle);
        }();

        return RecordHead{ size0 + size1, base, ext};
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
            rba,
            end_block,
            end_offset,
            std::move(bytes),
            header,
            ctx.over12c,
            ctx.isLittle,
            bound.boundInfo.vld == 0
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
        if (info) show(c.length_vector, dump);
    }

    void show(Record& r, uint8_t showMode, std::ostream &os) {
        fmt::println(os,
            "R {}{} ~{:>7}.@{:<3}{} | "
            "{}",
            coral::key_color, to_string(r.rba), r.end_block, r.end_offset, coral::reset_color,
            to_string(r.header)
        );

        if (showMode != 0) {
            fmt::println(os, "  * Change Count == {}", r.changes().size());
            for ( const auto& c : r.changes() ) {
                show(c, showMode, os);
            }
        }

    }

}