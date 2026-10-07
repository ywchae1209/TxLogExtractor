#pragma once
#include <fmt/format.h>
#include "tl/expected.hpp"
#include <variant>

#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_dlb_d.h"
#include "../elements/layout_dlb_l.h"

namespace ora {
    using coral::Result;
    using namespace combinator;

    using coral::decode_At, coral::Result, coral::err_of;
    using std::vector, std::array;

    // --------------------------------------------------------------------------------
    using DLB = std::variant<
        DLB_L,
        DLB_D
    >;

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
            auto o_block = ctx.one_of<DLB_D>("Ch19_1:DLB_D", DLB_D::decode);
            if (!o_block) return tl::make_unexpected(o_block.error());
            out.directBlock = std::move(*o_block);

            // [# 2~] unknown
            if (auto a = ctx.rest("Ch19_1:DLB_D:2") ) out.fields = std::move(*a);

            return out;

        } else {
            if (auto a = ctx.one_of<DLB_L>("Ch19_1:DLB_L", DLB_L::decode)) out.directBlock = std::move(*a);
            else return tl::make_unexpected(a.error());

            // [# 2~] unknown
            if (auto a = ctx.rest("Ch19_1:DLB_L:2") ) out.fields = std::move(*a);
            return out;
        }
    }

    // --------------------------------------------------------------------------------

    inline std::string to_string(const DLB &a) {
        return std::visit([](const auto &arg) -> std::string { return to_string(arg); }, a);
    }

    inline std::string to_string(Change_1901& a) {
        return fmt::format("Ch 19.1:\n  {}\n{}",
            to_string(a.directBlock),
            to_string(a.fields));
    }
}
