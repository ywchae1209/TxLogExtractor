#pragma once
#include "../coral_combinator.h"
#include "change_kdo.h"

namespace ora {
    using coral::Result;
    using namespace combinator;

    template <typename T>
    [[nodiscard]] inline Result<T> as_change(Ch_ktdo&& change) {
        if (auto* ptr = std::get_if<T>(&change)) {
            return std::move(*ptr);
        }
        return tl::make_unexpected(err_of("Type Mismatch"));
    }

    template<typename T>
    [[nodiscard]] inline Result<T> as_change(Result<Ch_ktdo> &&res) {
        if (!res)
            return tl::make_unexpected(res.error());
        if (auto *ptr = std::get_if<T>(&*res)) {
            return std::move(*ptr);
        }
        return tl::make_unexpected(err_of("Type Mismatch"));
    }

    // --------------------------------------------------------------------------------
    /// {11, 2, "KDBIRH", "Table redo: insert row header"}, (0x0B02 == Opcode 11.2)
    typedef Ch_Irp Change_1102;

    [[nodiscard]] inline Result<Change_1102> parse_1102( SpanCursor &ctx) {
        return as_change<Change_1102>(parse_ktdo(ctx, "Ch11_2", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 3, "KDBDRH", "Table redo: delete row header"},
    typedef Ch_Drp Change_1103;

    [[nodiscard]] inline Result<Change_1103> parse_1103( SpanCursor &ctx) {
        return as_change<Change_1103>(parse_ktdo(ctx, "Ch11_3", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 4, "KDBLKR", "Table redo: lock row"}, (0x0B04 == Opcode 11.4)
    typedef Ch_Lrk Change_1104;

    [[nodiscard]] inline Result<Change_1104> parse_1104( SpanCursor &ctx) {
        return as_change<Change_1104>(parse_ktdo(ctx, "Ch11_4", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 5, "KDBNUM", "Table redo: update row piece"}, (0x0B05 == Opcode 11.5)
    typedef Ch_Urp Change_1105;

    [[nodiscard]] inline Result<Change_1105> parse_1105( SpanCursor &ctx) {
        return as_change<Change_1105>(parse_ktdo(ctx, "Ch11_5", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 6, "KDBORP", "Table redo: overwrite row piece"}
    typedef Ch_Orp Change_1106;

    [[nodiscard]] inline Result<Change_1106> parse_1106( SpanCursor &ctx) {
        return as_change<Change_1106>(parse_ktdo(ctx, "Ch11_6", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 7, "KDBMFC", "Table redo: manipulate first column"}, (0x0B07 == Opcode 11.7)
    typedef Ch_Mfc Change_1107;

    [[nodiscard]] inline Result<Change_1107> parse_1107(SpanCursor &ctx) {
        return as_change<Change_1107>(parse_ktdo(ctx, "Ch11_7", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 8, "KDBCFA", "Table redo: change forwarding address"}, (0x0B08 == Opcode 11.8)
    typedef Ch_Cfa Change_1108;

    [[nodiscard]] inline Result<Change_1108> parse_1108(SpanCursor &ctx) {
        return as_change<Change_1108>(parse_ktdo(ctx, "Ch11_8", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 11, "KDBQMI", "Table redo: quick multi-insert"}, (0x0B0B == Opcode 11.11)
    typedef Ch_Qmi Change_1111;

    [[nodiscard]] inline Result<Change_1111> parse_1111(SpanCursor &ctx) {
        return as_change<Change_1111>(parse_ktdo(ctx, "Ch11_11", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 12, "KDBQMD", "Table redo: quick multi-delete"}
    typedef Ch_Qmd Change_1112;

    [[nodiscard]] inline Result<Change_1112> parse_1112(SpanCursor &ctx) {
        return as_change<Change_1112>(parse_ktdo(ctx, "Ch11_12", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 16, "KDOLMN", "LogMiner support RM for rowpiece with only logminer columns"},
    typedef Ch_Lmn Change_1116;

    [[nodiscard]] inline Result<Change_1116> parse_1116(SpanCursor &ctx) {
        return as_change<Change_1116>(parse_ktdo(ctx, "Ch11_16", ctx.isLittle));
    }

    // --------------------------------------------------------------------------------
    /// {11, 22, "KDBPDR", "Table redo: purge delete row"}, todo: KdoPurge/DrpBody (0x16 / 0x36)
    typedef Ch_Pdr Change_1122;

    [[nodiscard]] inline Result<Change_1122> parse_1122(SpanCursor &ctx) {
        return as_change<Change_1122>(parse_ktdo(ctx, "Ch11_22", ctx.isLittle));
    }
}
