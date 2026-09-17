#pragma once
#include <fmt/format.h>
#include <optional>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_5.h"
#include "tl/expected.hpp"

/// {5, 4, "KTURCM", "Commit transaction (transaction table update) - no undo record"},
/// todo :: rest

namespace ora {
    using coral::decode_At, coral::enough, coral::Result, coral::err_of;
    using std::optional, std::nullopt;
    using namespace combinator;

    /// {5, 4, "KTURCM", "Commit transaction (transaction table update) - no undo record"},
    /// - ucm :: Commit Header
    /// - ucf :: Commit Free/Space
    struct Change_0504 {
        Ktucm           ucm; // #1: Commit Header
        optional<Ktucf> ucf; // #2: Commit Free/Space

        constexpr bool has_ucf() const noexcept { return ucm.has_ucf() && ucf.has_value(); }
        constexpr bool is_rolled_back() const noexcept { return ucm.is_rolled_back(); }
    };
    // --------------------------------------------------------------------------------
    inline Result<Change_0504> parse_0504( SpanCursor &ctx) //, uint16_t usn_hint = 0)
    {
        Change_0504 out;

        // [# 1] ucm
        auto ucm = ctx.one<Ktucm>("Ch5_4:ucm", [&](auto s) { return decode_ktucm(s, ctx.isLittle); }); // usn_hint
        if (!ucm) return tl::make_unexpected(ucm.error());
        out.ucm = std::move(*ucm);


        // [# 2] ucf
        if (ucm->has_ucf() && ctx.no_remaining()) return err_of("Ch5_4: ucf flag is set, but no remaining.");

        auto ucf = ctx.one<Ktucf>("Ch5_4:ucf", [&](auto s) { return decode_ktucf(s, ctx.isLittle); });
        if (ucf) out.ucf = std::move(*ucf);

        return out;
    }
}