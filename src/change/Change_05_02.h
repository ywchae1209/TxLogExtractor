
#pragma once
#include <fmt/format.h>
#include <optional>
#include "../coral_combinator.h"
#include "../elements/layout_5.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {
    using coral::Result;
    using std::optional;
    using namespace combinator;

    /// {5, 2, "KTURDH", "Update rollback segment header"},
    /// - udh :: Undo Header
    /// - eop :: Extend Map Redo
    /// - pdb :: PDB id
    struct Change_0502 {
        Ktudh             udh; // #1
        optional<kteop>   eop; // #2  : Extent Map Redo
        optional<PdbInfo> pdb; // #2|3: PDB Info
    };

    // --------------------------------------------------------------------------------
    inline Result<Change_0502> parse_0502( SpanCursor &ctx ) {

        Change_0502 out;

        // [# 1] udh
        auto udh = ctx.one<Ktudh>("Ch5_2:udh", [&](auto s) { return decode_ktudh(s, ctx.isLittle); });
        if (!udh) return tl::make_unexpected(udh.error());
        out.udh = std::move(*udh);

        // [# 2,3] (eop?) ~ pdb
        if (const auto span2 = ctx.next("Ch5_2:pdb|eop"); span2) {

            //  if 4, --- note :: in OLR use over12 (not size)
            if (span2->size() == 4) {
                //  [# 2] pdb
                auto pdb2 = decode_pdb(*span2, ctx.isLittle);
                if (!pdb2) return tl::make_unexpected(pdb2.error());
                out.pdb = std::move(*pdb2);

            } else {
                // [# 2, 3] Kteop (36 bytes) ~ Pdb
                auto eop2 = decode_kteop(*span2, ctx.isLittle);
                if (!eop2) return tl::make_unexpected(eop2.error());
                out.eop = std::move(*eop2);

                // [# 3] if left, Pdb
                auto pdb3 = ctx.one<PdbInfo>("Ch5_2:pdb", [&](auto s) { return decode_pdb(s, ctx.isLittle); });
                if (!pdb3) return out;
                out.pdb = std::move(*pdb3);
            }
        }
        return out;
    }
}