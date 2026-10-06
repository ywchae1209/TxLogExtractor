#pragma once
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include <variant>

#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_oraBlock.h"

namespace ora {
    using coral::Result;
    using namespace combinator;

    using coral::decode_At, coral::Result, coral::err_of;
    using std::vector, std::array;

    ///  from OLR
    ///  Lob블록인 경우
    struct DLB_L {
        uint32_t           objd{0};        // Offset 0 : Data Object ID
        array<uint8_t, 10> lob_id{};       // Offset 4 : 10-Byte LOB ID
        uint32_t           v2{0};          // Offset 16: Version 2
        uint16_t           v1{0};          // Offset 20: Version 1
        uint32_t           lob_page_no{0}; // Offset 24: LOB Page Number
        uint32_t           pdba{0};        // Offset 28: Physical DBA ?? Previous DBA?
        size_t             lob_data_sz{0};
        vector<char>       lob_data;       // Offset 36~: Direct Payload Data

        static Result<DLB_L> decode( tcb::span<const char> buf, bool isLittle);
    };

    // ----------------------------------------------------------------------------------------------------
    inline Result<DLB_L> DLB_L::decode( tcb::span<const char> buf, bool isLittle) {

        if (buf.size() < 36) {
            return err_of(fmt::format("[DirectLoader19_1] buf size {} < 36", buf.size()));
        }

        DLB_L out;

        out.objd        = decode_At<uint32_t>(buf, isLittle, 0);
        std::memcpy(out.lob_id.data(), buf.data() + 4, 10);
        out.v2          = decode_At<uint32_t>(buf, isLittle, 16);
        out.v1          = decode_At<uint16_t>(buf, isLittle, 20);
        out.lob_page_no = decode_At<uint32_t>(buf, isLittle, 24);
        out.pdba        = decode_At<uint32_t>(buf, isLittle, 28);

        // 36~
        if (buf.size() > 36) {
            out.lob_data.assign(buf.begin() + 36, buf.end());
        }
        out.lob_data_sz = buf.size() - 36;

        return out;
    }
    // --------------------------------------------------------------------------------
    using DLB = std::variant<
        DLB_L,
        DLB_D
    >;


    // --------------------------------------------------------------------------------
    /// {19, 1, "KCBLCOLB", "Direct block logging"},
    /// - oraBlock :: Direct-Loaded OraBlock
    /// - elm2 :: unknown -- todo
    struct Change_1901 {

        DLB   directBlock;
        RawFlds fields;

        static Result<Change_1901> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_1901> Change_1901::parse(SpanCursor &ctx) {

        Change_1901 out;

        auto span1 = ctx.peek();
        if (!span1) return tl::make_unexpected(span1.error());

        auto is_data = DLB_D::is_data_block(*span1);

        if (is_data) {

            // [# 1] OraBlock
            auto o_block = ctx.one_of<DLB_D>("Ch19_1:DLB_D", decode_ora_block);
            if (!o_block) return tl::make_unexpected(o_block.error());
            out.directBlock = std::move(*o_block);

            // [# 2~] unknown
            if (auto a = ctx.rest("Ch19_1:DLB_D:fields") ) out.fields = std::move(*a);

            return out;

        } else {
            if (auto a = ctx.one_of<DLB_L>("Ch19_1:DLB_L", DLB_L::decode)) out.directBlock = std::move(*a);
            else return tl::make_unexpected(a.error());

            // [# 2~] unknown
            if (auto a = ctx.rest("Ch19_1:DLB_L:fields") ) out.fields = std::move(*a);
            return out;
        }
    }

    inline std::string to_string(const DLB_L& a) {

        std::string lob_id_str;
        lob_id_str.reserve(20);
        for (uint8_t b : a.lob_id) {
            lob_id_str += fmt::format("{:02x}", b);
        }

        std::string out = fmt::format(
            "DLB_LF: objd: 0x{:08x}, LobId: 0x{}, page: {}, version: 0x{:04x}.{:08x}, pdba: 0x{:08x}, sz: {}\n    ",
            a.objd, lob_id_str, a.lob_page_no, a.v1, a.v2, a.pdba, a.lob_data_sz );

        // 36 ~
        if (!a.lob_data.empty()) {
            auto min = (16 * 6) < a.lob_data.size() ? 16 * 6 : a.lob_data.size();
            for (size_t j = 0; j < min; ++j) {
                out += fmt::format("{:02x} ", static_cast<uint8_t>(a.lob_data[j]));
                if ((j % 32) == 31 && j != a.lob_data.size() - 1) {
                    out += "\n    ";
                }
            }
            out += '\n';
        }

        return out;
    }


    inline std::string to_string(const DLB_D& a) {
        return "DLB_D : todo";
    }

    inline std::string to_string(const DLB &a) {
        return std::visit([](const auto &arg) -> std::string { return to_string(arg); }, a);
    }


    inline std::string to_string(Change_1901& a) {
        return fmt::format("Ch 19.1:\n  {}\n{}",
            to_string(a.directBlock),
            to_string(a.fields));
    }
}
