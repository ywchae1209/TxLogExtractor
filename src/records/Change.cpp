#include "layout_ChangeHead.h"
#include "ora_opCodes.h"
#include "Record.h"
#include "../coral_result.h"

namespace ora {

    using fmt::format;
    using coral::get_at;
    using coral::err_of;

    // --------------------------------------------------------------------------------
    static auto decode_ChangeHead(const tcb::span<const char> &raw,
                                  const bool over12c,
                                  const bool isLittle)
        -> coral::Result<std::tuple<tcb::span<const char>, ChangeHead> > {

        constexpr auto lo_sz = sizeof(ChangeHead_base_lo);
        constexpr auto ex_sz = sizeof(ChangeHead_ext_lo);

        const auto sz = lo_sz + (over12c ? ex_sz : 0);

        if (raw.size() < sz)
            return err_of( format("ChangeHead: not enough {} < {}(need) ", raw.size(), sz));

        auto c = decode_change_head(raw, over12c, isLittle);

        const auto [cls, usn] = get_cls_usn(c.base.cls);

        const uint16_t afn      = c.base.afn_obj       & 0xFFFFu;
        const uint16_t obj_high = c.base.afn_obj >> 16 & 0xFFFFu;
        const uint16_t obj_low  = c.base.obj_low;
        const uint32_t obj_id   = static_cast<uint32_t>(obj_high) << 16 | obj_low;

        ChangeHead o{
            .span     = raw.first(sz),
            .size     = sz,
            .opLayer  = c.base.opLayer,
            .opCode   = c.base.opCode,
            .cls      = cls,
            .usn      = usn,
            .afn      = afn,
            .obj_id   = obj_id,
            .obj_low  = obj_low,
            .obj_high = obj_high,
            .dba      = c.base.dba,
            .rfile_no = static_cast<uint16_t>(c.base.dba >> 22),          // High 10 bits
            .block_no = c.base.dba & 0x003FFFFFu,                         // Low 22 bits
            .scn      = c.base.scn,
            .seq      = c.base.seq,
            .ctype    = c.base.ctype,
            .con_id   = c.ext ? std::make_optional(c.ext->con_id) : std::nullopt
        };

        return std::make_tuple(raw.subspan(sz), std::move(o));
    }

    // --------------------------------------------------------------------------------
    static auto decode_LengthVector(const tcb::span<const char> &raw, const bool isLittle)
        -> coral::Result<std::tuple<tcb::span<const char>, LengthVector> > {


        const auto raw_sz = raw.size();

        if (raw_sz < 4) // at least 4 byte. even if lv0 == 0.
            return err_of("LengthVector: not enough");

        // raw[0] = Length of Total Length-Array
        const auto lv0 = get_at<uint16_t>(raw, 0, isLittle);
        if (lv0 == 0)
            return std::make_tuple(raw.subspan(4), LengthVector{});

        if (lv0 > raw_sz || lv0 % 2 != 0)
            return err_of(format("LengthVector: invalid lv0 {}", lv0));

        const auto slots = lv0 / sizeof(uint16_t);
        const auto e_count = slots - 1;

        std::vector<uint16_t> o_sizes;
        std::vector<tcb::span<const char>> o_spans;

        o_sizes.reserve(e_count);
        o_spans.reserve(e_count);

        auto e_offset = coral::align_up4(lv0); // 4-byte align.

        for (auto i = 1; i < slots; ++i) {

            const auto e_sz = get_at<uint16_t>(raw, i * sizeof(uint16_t), isLittle);

            if (e_offset + e_sz > raw_sz) {
                return err_of(format("LengthVector: invalid lv{}/{}: {} left {}", i, slots, e_sz, raw_sz - e_offset));
            }

            o_sizes.push_back(e_sz);
            o_spans.push_back(raw.subspan(e_offset, e_sz));

            e_offset += coral::align_up4(e_sz);  // 4-byte align-up
        }

        return std::make_tuple(raw.subspan(e_offset),
                               LengthVector{
                                   .sizes = std::move(o_sizes),
                                   .spans = std::move(o_spans) });
    }

    // --------------------------------------------------------------------------------
    auto Changes_of(const RBA &rba,
                    const tcb::span<const char> &raw,
                    const bool over12c,
                    const bool isLittle,
                    std::vector<Change>& out)
        -> coral::Result<bool> {

        tcb::span<const char> view = raw;

        auto idx = 0;
        while (!view.empty()) {

            if (idx > 256)[[unlikely]] {
                return err_of(format("[Changes_of] {} : too much Changes {}", to_string(rba), idx));
            }

            // ----------------------------------------
            auto ch_res = decode_ChangeHead(view, over12c, isLittle);
            if (!ch_res) [[unlikely]] {
                return err_of( format("[Changes_of] {} idx [{}] {}", to_string(rba), idx, ch_res.error()));
            }
            auto [s1, ch] = std::move(*ch_res);

            // ----------------------------------------
            auto lv_res = decode_LengthVector(s1, isLittle);
            if (!lv_res) [[unlikely]] {
                return err_of( format("[Changes_of] {} idx [{}] {}", to_string(rba), idx, lv_res.error()));
            }
            auto [s2, lv] = std::move(*lv_res);

            out.push_back(Change{
                .change_head    = std::move(ch),
                .length_vector  = std::move(lv)
            });

            view = s2;
            idx++;
        }
        return true;
    }

    // --------------------------------------------------------------------------------
    std::string to_string(BlockClassType cls) {
        switch (cls) {
            case BlockClassType::DataBlock:             return {"Data_block"};
            case BlockClassType::SortBlock:             return {"Sort_block"};
            case BlockClassType::SaveUndoBlock:         return {"Save_undo_block"};
            case BlockClassType::SegmentHeader:         return {"Segment_header"};
            case BlockClassType::SaveUndoHeader:        return {"Save_undo_header"};
            case BlockClassType::FreeListBlock:         return {"Free_list_block"};
            case BlockClassType::ExtentMapBlock:        return {"Extent_map_block"};
            case BlockClassType::Bmb1st:                return {"1st_level_bmb"};
            case BlockClassType::Bmb2nd:                return {"2nd_level_bmb"};
            case BlockClassType::Bmb3rd:                return {"3rd_level_bmb"};
            case BlockClassType::BitmapBlock:           return {"Bitmap_block"};
            case BlockClassType::BitmapIndexBlock:      return {"Bitmap_index_block"};
            case BlockClassType::FileHeaderBlock:       return {"File_header_block"};
            case BlockClassType::DeferredRollbackBlock: return {"Deferred_rollback_block"};
            case BlockClassType::SystemUndoHeader:      return {"SYSTEM_Undo_header"};
            case BlockClassType::SystemUndoBlock:       return {"SYSTEM_Undo_block"};
            case BlockClassType::UndoHeader:            return {"Undo_Header"};
            case BlockClassType::UndoBlock:             return {"Undo_Block"};
            default:                                    return {"Unknown"};
        }
    }

    // --------------------------------------------------------------------------------
    std::string to_string(const ChangeHead& h) {

        const auto desc = opCode_string(h.opLayer, h.opCode);
        const auto ctype_str = cType_string(h.opLayer, h.opCode, h.ctype);
        const auto con_str = h.con_id.has_value() ? fmt::format("ConID: {}", *h.con_id) : "";
        const auto usn_str = h.usn != 0 ? fmt::format("USN: {}", h.usn) : "";

        return fmt::format(
          "{}  ├─ {}{}\n"
            "  ├─ {}\n"
            "  ├─ CLS: {}\n"
            "  ├─ AFN: {} [RFN: {} Block#: {}] = (DBA: {:#010x}) {}\n"
            "  ├─ Obj: {} ({:#06x}.{:#06x})\n"
            "  └─ SCN: {} Seq: {} {}",
          coral::Rev_st, desc, coral::Rev_End,
          ctype_str,
          to_string(h.cls),
          h.afn, h.rfile_no, h.block_no, h.dba, usn_str,
          h.obj_id, h.obj_high, h.obj_low,
          toHex(h.scn), h.seq,
          con_str
        );
    }

    // --------------------------------------------------------------------------------
    void show(const LengthVector &lv, bool dump) {
        for (size_t i = 0; i < lv.spans.size(); ++i) {
            fmt::println("  * [#{}] ({} bytes)", i + 1, lv.spans[i].size());
            if (dump)
                coral::show_HexDump(lv.spans[i]);
        }
    }
}
