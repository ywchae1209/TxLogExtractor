#include "layout_ChangeHead.h"
#include "ora_opCodes.h"
#include "Record.h"

namespace ora {

    // --------------------------------------------------------------------------------
    static auto decode_ChangeHead(const tcb::span<const char> &raw,
                           const bool over12c,
                           const bool isLittle) -> std::tuple<tcb::span<const char>, ChangeHead> {

        using coral::decode;

        constexpr auto lo_sz = sizeof(ChangeHead_lo);
        constexpr auto ex_sz = sizeof(ChangeHead_ext_lo);

        const auto sz = lo_sz + (over12c ? ex_sz : 0);

        if (raw.size() < sz)
            throw std::out_of_range(fmt::format("[decode_ChangeHead]: not enough {} < {}(need) ", raw.size(), sz));

        const auto lo0 = [&]() -> ChangeHead_lo {
            ChangeHead_lo ch{};
            std::memcpy(&ch, raw.data(), lo_sz);
            return ch;
        }();

        //// variable Header : over12c
        const auto ext = [&]() -> std::optional<ChangeHead_ext_lo> {
            if (!over12c) return std::nullopt;
            ChangeHead_ext_lo ex{};
            std::memcpy(&ex, raw.data() + lo_sz, ex_sz);
            return ex;
        }();

        auto o = decode(lo0, ext, raw.subspan(0, sz), isLittle);

        return std::make_tuple(raw.subspan(sz), o);
    }

    // --------------------------------------------------------------------------------
    static constexpr size_t align_up4(const size_t size) noexcept {
        return (size + 3) & ~static_cast<size_t>(3);
    }

    using coral::decode;

    static auto decode_LengthVector(const tcb::span<const char> &raw,
                                    const bool isLittle) -> std::tuple<tcb::span<const char>, LengthVector> {

        if (raw.size() < sizeof(uint16_t))
            throw std::out_of_range("decode_LengthVector:1: not enough");

        // raw[0] = Length of Total Length-Array
        uint16_t lv0;
        std::memcpy(&lv0, raw.data(), sizeof(uint16_t));
        const auto lv_len = decode(lv0, isLittle);

        if (lv_len == 0) {
            return std::make_tuple(raw.subspan(4), LengthVector{});
        }

        const bool valid_lv_len = lv_len >= sizeof(uint16_t)
                               && lv_len <= raw.size()
                               && lv_len % 2 == 0;      // uint16_t

        if (!valid_lv_len) {
            auto msg = fmt::format( "decode_LengthVector:2: wrong lv0: {} (decoded){} : {} ", lv0, lv_len, raw.size() );
            fmt::println(std::cerr, msg);
            coral::show_HexDump(raw);
            throw std::out_of_range( msg);
        }

        // slot
        const auto slots = lv_len / sizeof(uint16_t);

        // sizes
        std::vector<uint16_t> o_sizes;
        o_sizes.reserve(slots-1);

        for (auto i = 1; i < slots; ++i) {
            uint16_t lv;
            std::memcpy(&lv, raw.data() + i * sizeof(uint16_t), sizeof(uint16_t));
            o_sizes.push_back( decode(lv, isLittle));
        }

        auto offset = align_up4(lv_len); // 4-byte align.

        // sizes
        std::vector<tcb::span<const char>> o_spans;
        o_spans.reserve(slots-1);

        for (auto i = 1; i < slots; ++i) {

            const auto elm = o_sizes[i-1];
            if (offset + elm > raw.size()) {
                throw std::out_of_range(
                      fmt::format("decode_LengthVector:3: {} -> {} : {} {}/{} {} != {}",
                                  lv0, lv_len, raw.size(), i, slots, elm, raw.size()-offset)
                );
            }
            o_spans.push_back(raw.subspan(offset, elm));

            offset += align_up4(elm);       // 4-byte align
        }

        return std::make_tuple(raw.subspan(offset),
                               LengthVector{std::move(o_sizes), std::move(o_spans)});
    }

    // --------------------------------------------------------------------------------
    auto Changes_of(const RBA &rba,
                    const tcb::span<const char> &raw,
                    const bool over12c,
                    const bool isLittle) -> std::vector<Change> {

        std::vector<Change> changes;
        tcb::span<const char> view = raw;

        auto idx = 0;
        while (!view.empty()) {
            if (idx > 256)
                throw std::out_of_range(fmt::format("[Changes_of] {} too much..", to_string(rba)) );

            try {
                auto [s1, ch] = decode_ChangeHead(view, over12c, isLittle);
                auto [s2, lv] = decode_LengthVector(s1, isLittle);

                changes.push_back( Change{ ch, std::move(lv) });
                view = s2;

            } catch (const std::exception &e) {
                throw std::runtime_error(fmt::format("[Changes_of] {} idx {} {}", to_string(rba), idx, e.what()));
                // coral::show_HexDump( view, std::cerr);
            }
            idx++;
        }
        return changes;
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
