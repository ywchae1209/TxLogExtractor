#pragma once
#include <string>
#include <string_view>
#include <optional>
#include <fmt/format.h>
#include "tcb/span.hpp"
#include "tl/expected.hpp"
#include "../coral_decode.h"
#include "../coral_combinator.h"
#include "../elements/layout_5.h"

namespace ora {
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {5, 30, "KTURCDTS", "Change on-disk state for a distributed transaction (KTUDX)"},
    struct Change_0530 {
        Ktudx            udx;
        optional<RawFld> ext_redo_meta;
        optional<RawFld> global_tx_state;
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_0530> parse_0530( SpanCursor &ctx ) {

        Change_0530 out;

        // [# 1] udx
        auto udx = ctx.one<Ktudx>("Change5_30:udx", [&](auto s) { return decode_ktudx(s, ctx.isLittle); });
        if (!udx) return tl::make_unexpected(udx.error());
        out.udx = *udx;

        // [# 2, 3] ExtRedoMeta ~ Global Tx State
        if (auto r = ctx.raw("Ch10_2:f2"); r) out.ext_redo_meta = std::move(*r); else return out;
        if (auto r = ctx.raw("Ch10_2:f3"); r) out.global_tx_state= std::move(*r); else return out;

        return out;
    }

    // --------------------------------------------------------------------------------
    inline std::string to_string(const Change_0530 &c) {

        std::string out;
        fmt::format_to(std::back_inserter(out),
                       "Change5_30{{xid_slt={}, xid_sqn={}, sta={:#04x}, cfl={:#04x}",
                       c.udx.xid_slt,
                       c.udx.xid_sqn,
                       c.udx.sta,
                       c.udx.cfl);

        if (c.ext_redo_meta) fmt::format_to(std::back_inserter(out), ", ext_meta_sz={}", c.ext_redo_meta->bytes.size());
        if (c.global_tx_state) fmt::format_to(std::back_inserter(out), ", global_state_sz={}", c.global_tx_state->bytes.size());

        out += "}";
        return out;
    }

}