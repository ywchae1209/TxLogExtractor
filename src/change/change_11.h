#pragma once
#include "../coral_combinator.h"
#include "change_kdo.h"

namespace ora {
    using coral::Result;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// {11, 2, "KDBIRH", "Table redo: insert row header"}, (0x0B02 == Opcode 11.2)
    struct Change_1102 : Ch_Irp {
        using Ch_Irp::Ch_Irp;
        explicit Change_1102(Ch_Irp &&h) noexcept : Ch_Irp(std::move(h)) {}

        static Result<Change_1102> parse(SpanCursor &ctx) {
            return from<Change_1102, Ch_Irp>(parse_kdop(ctx, "Ch11_2", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 3, "KDBDRH", "Table redo: delete row header"},
    struct Change_1103 : Ch_Drp {
        using Ch_Drp::Ch_Drp;
        explicit Change_1103(Ch_Drp &&h) noexcept : Ch_Drp(std::move(h)) {}
        static Result<Change_1103> parse(SpanCursor &ctx) {
            return from<Change_1103, Ch_Drp>(parse_kdop(ctx, "Ch11_3", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 4, "KDBLKR", "Table redo: lock row"}, (0x0B04 == Opcode 11.4)
    struct Change_1104 : Ch_Lrk {
        using Ch_Lrk::Ch_Lrk;
        explicit Change_1104(Ch_Lrk &&h) noexcept : Ch_Lrk(std::move(h)) {}
        static Result<Change_1104> parse(SpanCursor &ctx) {
            return from<Change_1104, Ch_Lrk>(parse_kdop(ctx, "Ch11_4", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 5, "KDBNUM", "Table redo: update row piece"}, (0x0B05 == Opcode 11.5)
    struct Change_1105 : Ch_Urp {
        using Ch_Urp::Ch_Urp;
        explicit Change_1105(Ch_Urp &&h) noexcept : Ch_Urp(std::move(h)) {}
        static Result<Change_1105> parse(SpanCursor &ctx) {
            return from<Change_1105, Ch_Urp>(parse_kdop(ctx, "Ch11_5", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 6, "KDBORP", "Table redo: overwrite row piece"}
    struct Change_1106 : Ch_Orp {
        using Ch_Orp::Ch_Orp;
        explicit Change_1106(Ch_Orp &&h) noexcept : Ch_Orp(std::move(h)) {}
        static Result<Change_1106> parse(SpanCursor &ctx) {
            return from<Change_1106, Ch_Orp>(parse_kdop(ctx, "Ch11_6", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 7, "KDBMFC", "Table redo: manipulate first column"}, (0x0B07 == Opcode 11.7)
    struct Change_1107 : Ch_Mfc {
        using Ch_Mfc::Ch_Mfc;
        explicit Change_1107(Ch_Mfc &&h) noexcept : Ch_Mfc(std::move(h)) {}
        static Result<Change_1107> parse(SpanCursor &ctx) {
            return from<Change_1107, Ch_Mfc>(parse_kdop(ctx, "Ch11_7", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 8, "KDBCFA", "Table redo: change forwarding address"}, (0x0B08 == Opcode 11.8)
    struct Change_1108 : Ch_Cfa {
        using Ch_Cfa::Ch_Cfa;
        explicit Change_1108(Ch_Cfa &&h) noexcept : Ch_Cfa(std::move(h)) {}
        static Result<Change_1108> parse(SpanCursor &ctx) {
            return from<Change_1108, Ch_Cfa>(parse_kdop(ctx, "Ch11_8", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 11, "KDBQMI", "Table redo: quick multi-insert"}, (0x0B0B == Opcode 11.11)
    struct Change_1111 : Ch_Qmi {
        using Ch_Qmi::Ch_Qmi;
        explicit Change_1111(Ch_Qmi &&h) noexcept : Ch_Qmi(std::move(h)) {}
        static Result<Change_1111> parse(SpanCursor &ctx) {
            return from<Change_1111, Ch_Qmi>(parse_kdop(ctx, "Ch11_11", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 12, "KDBQMD", "Table redo: quick multi-delete"}
    struct Change_1112 : Ch_Qmd {
        using Ch_Qmd::Ch_Qmd;
        explicit Change_1112(Ch_Qmd &&h) noexcept : Ch_Qmd(std::move(h)) {}
        static Result<Change_1112> parse(SpanCursor &ctx) {
            return from<Change_1112, Ch_Qmd>(parse_kdop(ctx, "Ch11_12", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 16, "KDOLMN", "LogMiner support RM for rowpiece with only logminer columns"},
    struct Change_1116 : Ch_Lmn {
        using Ch_Lmn::Ch_Lmn;
        explicit Change_1116(Ch_Lmn &&h) noexcept : Ch_Lmn(std::move(h)) {}
        static Result<Change_1116> parse(SpanCursor &ctx) {
            return from<Change_1116, Ch_Lmn>(parse_kdop(ctx, "Ch11_16", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    /// {11, 22, "KDBPDR", "Table redo: purge delete row"}, todo: check :: body type not-certain
    struct Change_1122 : Ch_Drp {
        using Ch_Drp::Ch_Drp;
        explicit Change_1122(Ch_Drp &&h) noexcept : Ch_Drp(std::move(h)) {}
        static Result<Change_1122> parse(SpanCursor &ctx) {
            return from<Change_1122, Ch_Drp>(parse_kdop(ctx, "Ch11_22", ctx.isLittle));
        }
    };

    // --------------------------------------------------------------------------------
    static std::string to_string(const Change_1102 &c) {
        return fmt::format("Ch 11.2: {}", to_string(static_cast<const Ch_Irp &>(c)));
    }
    static std::string to_string(const Change_1103 &c) {
        return fmt::format("Ch 11.3: {}", to_string(static_cast<const Ch_Drp &>(c)));
    }
    static std::string to_string(const Change_1104 &c) {
        return fmt::format("Ch 11.4: {}", to_string(static_cast<const Ch_Lrk &>(c)));
    }
    static std::string to_string(const Change_1105 &c) {
        return fmt::format("Ch 11.5: {}", to_string(static_cast<const Ch_Urp &>(c)));
    }
    static std::string to_string(const Change_1106 &c) {
        return fmt::format("Ch 11.6: {}", to_string(static_cast<const Ch_Orp &>(c)));
    }
    static std::string to_string(const Change_1107 &c) {
        return fmt::format("Ch 11.7: {}", to_string(static_cast<const Ch_Mfc &>(c)));
    }
    static std::string to_string(const Change_1108 &c) {
        return fmt::format("Ch 11.8: {}", to_string(static_cast<const Ch_Cfa &>(c)));
    }
    static std::string to_string(const Change_1111 &c) {
        return fmt::format("Ch 11.11: {}", to_string(static_cast<const Ch_Qmi &>(c)));
    }
    static std::string to_string(const Change_1112 &c) {
        return fmt::format("Ch 11.12: {}", to_string(static_cast<const Ch_Qmd &>(c)));
    }
    static std::string to_string(const Change_1116 &c) {
        return fmt::format("Ch 11.16: {}", to_string(static_cast<const Ch_Lmn &>(c)));
    }
    static std::string to_string(const Change_1122 &c) {
        return fmt::format("Ch 11.22: {}", to_string(static_cast<const Ch_Drp &>(c)));
    }
}
