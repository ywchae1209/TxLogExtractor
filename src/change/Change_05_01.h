#pragma once

#include <fmt/format.h>
#include <optional>
#include <variant>
#include <vector>
#include "../coral_combinator.h"
#include "../elements/layout_5.h"
#include "../elements/layout_5_ktub.h"
#include "change_kdo.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

/// {5, 1, "KTUUNDO", "Transaction Undo Change Vector"}, (0x0501 == Opcode 5.1)

namespace ora {
    using coral::decode_at, coral::decode_At, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::vector, std::variant;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// KDO Undo (Before Image & Supplemental Logging)
    struct KdoUndo {
        Ch_ktdo          ktdo; // ktb, kdo
        optional<Ch_sup> uspl;
    };

    struct KliUndo {
        RawFlds rest;
    };

    struct OtherUndo {
        RawFlds rest;
    };

    /// Before Image & Supplemental Logging
    using KtuBody = std::variant<std::monostate, //
                                 KdoUndo,        //
                                 KliUndo,        //
                                 OtherUndo >;

    // --------------------------------------------------------------------------------
    struct Change_0501 {
        Ktudb udb;                   // # 1: KTU Undo Block Header (contain xid)
        Ktub  ub;                    // # 2: KTU Block Header

        KtuBody before{};

        uint32_t objn() const { return ub.header.objn; }
        uint32_t objd() const { return ub.header.objd; }
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_0501> parse_0501( SpanCursor& ctx ) {

        // [# 1] udb (Undo Header)
        auto udb = ctx.one<Ktudb>( "Ch5_1:udb", [&](auto s) { return decode_ktudb(s, ctx.isLittle); });
        if (!udb) return tl::make_unexpected(udb.error());

        // [# 2] ub (Undo Block Header)
        auto ub = ctx.one<Ktub>("Ch5_1:ub", [&](auto s) { return decode_ktub(s, ctx.isLittle, false); });
        if (!ub) return tl::make_unexpected(ub.error());

        Change_0501 out {
            .udb = *udb,
            .ub = *ub
        };

        // MBU (Multi-Block Undo)
        if ((out.ub.header.flg & (Ktub_Flag::MBU_HEAD | Ktub_Flag::MBU_TAIL | Ktub_Flag::MBU_MID)) != 0) {
            return out;
        }

        const uint16_t op = out.ub.header.opc;
        switch (op) {
            // 11.1 --> op 11.x :: KDO Undo (Row Before Image )
            // [# 3] Ktb ~ Kdo ~ ...  (0x0E08 Truncate 이외 파싱)
            case 0x0B01: {
                KdoUndo undo{};
                if (auto kdo = parse_ktdo(ctx, "Ch5_1:ktdo", ctx.isLittle)) undo.ktdo = std::move(*kdo);
                if (auto sup = parse_ksup(ctx, "Ch5_1:uspl", ctx.isLittle)) undo.uspl = std::move(*sup);
                out.before = std::move(undo);
                return out;
            }
                // 26.1 --> KDLI Undo (LOB Undo) ::: todo
            case 0x1A01: {
                KliUndo undo{};
                // ktb
                // kdliHead
                // kdliElem
                if (auto r = ctx.rest(""); r)
                    undo.rest = std::move(*r);
                out.before = std::move(undo);
                return out;
            }

                // todo :::
            case 0x1A16: {
                KliUndo undo{};

                if (auto r = ctx.rest(""); r)
                    undo.rest = std::move(*r);
                out.before = std::move(undo);
                return out;
            }
                // todo :::
            default: {
                OtherUndo undo{};
                if (auto r = ctx.rest(""); r)
                    undo.rest = std::move(*r);
                out.before = std::move(undo);
                return out;
            }
        }
    }
}
