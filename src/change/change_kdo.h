#pragma once

#include <fmt/format.h>
#include <string>

#include "../coral_combinator.h"
#include "../elements/layout_kdo.h"
#include "../elements/layout_ktb.h"
#include "change_usp.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::Result, coral::err_of;
    using namespace combinator;

    /// Data Operation Header -- KTB ~ KDO
    struct Ch_DoH {
        std::optional<KtbVector> ktb; // # 1
        KdoVector kdo;                // # 2

        size_t span_remain;     // todo :: g3nie

        bool is_kdom2() const noexcept { return kdo.head.is_kdom2(); }
        bool is_rowDependencies() const noexcept { return ora::is_rowDependencies(kdo.head.op); }
        bool isCompressed(size_t span_size) const { return ora::isCompressed(kdo.body, span_size); }

        static Result<Ch_DoH> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    struct Ch_Lrk : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Lrk(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Qmd : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Qmd(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };

    struct Ch_Cki : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Cki(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Skl : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Skl(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Dsc : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Dsc(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Raw : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Raw(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };

    struct Ch_Cmp : Ch_DoH {
        using Ch_DoH::Ch_DoH;

        std::optional<RawFld> payload0;
        std::optional<RawFld> payload1;

        explicit Ch_Cmp(Ch_DoH &&h,
                        std::optional<RawFld> p0 = std::nullopt,
                        std::optional<RawFld> p1 = std::nullopt) noexcept :
            Ch_DoH(std::move(h)), payload0(std::move(p0)), payload1(std::move(p1)) {}

        static Result<Ch_Cmp> parse(SpanCursor &ctx, Ch_DoH &&h) {
            switch (ctx.remaining()) {
                case 0: return Ch_Cmp{std::move(h)};
                case 1: return Ch_Cmp{std::move(h), std::move(*ctx.one_raw("Cmp:p0"))};
                default: return Ch_Cmp{std::move(h), std::move(*ctx.one_raw("Cmp:p0")), std::move(*ctx.one_raw("Cmp:p1"))};
            }
        }
    };

    struct Ch_Mfc : Ch_DoH {
        using Ch_DoH::Ch_DoH;

        std::optional<RawFld> payload0;
        std::optional<RawFld> payload1;

        explicit Ch_Mfc(Ch_DoH &&h,
                        std::optional<RawFld> p0 = std::nullopt,
                        std::optional<RawFld> p1 = std::nullopt) noexcept :
            Ch_DoH(std::move(h)), payload0(std::move(p0)), payload1(std::move(p1)) {}

        static Result<Ch_Mfc> parse(SpanCursor &ctx, Ch_DoH &&h) {
            switch (ctx.remaining()) {
                case 0: return Ch_Mfc{std::move(h)};
                case 1: return Ch_Mfc{std::move(h), std::move(*ctx.one_raw("mfc:p0"))};
                default: return Ch_Mfc{std::move(h), std::move(*ctx.one_raw("mfc:p0")), std::move(*ctx.one_raw("mfc:p1"))};
            }
        }
    };

    struct Ch_Cfa : Ch_DoH {
        using Ch_DoH::Ch_DoH;

        std::optional<Ch_Usp> spl;

        explicit Ch_Cfa(Ch_DoH &&h, std::optional<Ch_Usp> s = std::nullopt) noexcept
        : Ch_DoH(std::move(h)), spl{std::move(s)} {}


        static Result<Ch_Cfa> parse(SpanCursor &ctx, Ch_DoH &&h) {
            if (auto a = Ch_Usp::parse(ctx, "cfa:sup")) return Ch_Cfa{std::move(h), std::move(*a)};
            else return Ch_Cfa { std::move(h)};
        }
    };

    struct Ch_Lmn : Ch_DoH {
        using Ch_DoH::Ch_DoH;

        std::optional<Ch_Usp> spl;

        explicit Ch_Lmn(Ch_DoH &&h, std::optional<Ch_Usp> s = std::nullopt) noexcept
        : Ch_DoH(std::move(h)), spl{std::move(s)} {}

        static Result<Ch_Lmn> parse(SpanCursor &ctx, Ch_DoH &&h) {

            if (auto a = Ch_Usp::parse(ctx, "cfa:sup")) return Ch_Lmn{std::move(h), std::move(*a)};
            else return Ch_Lmn { std::move(h)};
        }

    };
    // --------------------------------------------------------------------------------
    struct Ch_Drp : Ch_DoH {
        using Ch_DoH::Ch_DoH;

        std::optional<uint64_t> dscn; // row-dependency scn

        // ----------------------------------------
        explicit Ch_Drp(Ch_DoH &&h, std::optional<uint64_t> d = std::nullopt) noexcept
        : Ch_DoH(std::move(h)), dscn{d} {}

        static Result<Ch_Drp> parse(SpanCursor &ctx, Ch_DoH &&h) {
            return Ch_Drp{
                std::move(h),
                ctx.one_scn8_if("drp:scn", h.is_rowDependencies())};
        }
    };

    struct Ch_Irp : Ch_DoH {
        using Ch_DoH::Ch_DoH;

        RawFlds col_raws;
        std::optional<uint64_t> dscn;  // row-dependency scn
        bool compressed;

        // ----------------------------------------
        explicit Ch_Irp(Ch_DoH &&h, RawFlds &&r, std::optional<uint64_t> d, bool isCompressed = false) noexcept
        : Ch_DoH(std::move(h)), col_raws(std::move(r)), dscn{d}, compressed {isCompressed} {}

        static Result<Ch_Irp> parse(SpanCursor &ctx, Ch_DoH &&h) {

            auto s3 = ctx.peek();
            if (!s3) return Ch_Irp{ std::move(h), RawFlds{}, std::nullopt };

            const auto compressed = h.isCompressed(s3->size());

            // [# 1 ~ N] Column Data Fields : cc
            auto raws = compressed ? ctx.one_as_raws("irp:cols:comp")
                                   : ctx.n_raws("irp:cols:n", get_cc(h.kdo.body));

            if (!raws) return tl::make_unexpected(raws.error());

            auto dscn = ctx.one_scn8_if("irp:dscn", h.is_rowDependencies());

            return Ch_Irp{
                std::move(h),
                std::move(*raws),
                dscn, compressed};
        }
    };

    struct Ch_Urp : Ch_DoH {
        using Ch_DoH::Ch_DoH;

        RawFlds col_raws;
        std::vector<uint16_t> col_indices; // colVector | colElements
        std::optional<uint64_t> dscn;      // row-dependency scn

        explicit Ch_Urp(Ch_DoH &&h, std::vector<uint16_t> &&idx, RawFlds &&r, std::optional<uint64_t> d) noexcept
            : Ch_DoH(std::move(h)), col_raws(std::move(r)), col_indices(std::move(idx)), dscn{d} {}

        static Result<Ch_Urp> parse(SpanCursor &ctx, Ch_DoH &&h) {

            const auto nnew = get_nnew(h.kdo.body);

            // [# 1] Column Index Array : nnew
            auto indices = ctx.one_array<uint16_t>("urp:c-nums", nnew);
            if (!indices) return tl::make_unexpected(indices.error());

            // [# 2]
            auto raws = h.is_kdom2()
                        ? ctx.one_raws_by("urp:col_vec", *indices) // [# 4]   Col-Vector
                        : ctx.n_raws("urp:col_raw", nnew);         // [# 4 ~] Col-Elements

            if (!raws) return tl::make_unexpected(raws.error());

            // [~]
            auto dscn = ctx.one_scn8_if("urp:dscn", h.is_rowDependencies());

            return Ch_Urp{
                std::move(h),
                std::move(*indices),
                std::move(*raws),
                dscn
            };
        }
    };

    struct Ch_Orp : Ch_DoH {
        RawFlds col_raws;
        std::optional<uint64_t> dscn; // row-dependency scn
        bool compressed;

        explicit Ch_Orp(Ch_DoH &&h, RawFlds &&r, std::optional<uint64_t> d, bool isCompressed = false) noexcept
            : Ch_DoH(std::move(h)), col_raws(std::move(r)), dscn{d}, compressed {isCompressed} {}

        static Result<Ch_Orp> parse(SpanCursor &ctx, Ch_DoH &&h) {

            auto s3 = ctx.peek();
            if (!s3) return Ch_Orp{ std::move(h), RawFlds{}, std::nullopt };

            const auto compressed = h.isCompressed(s3->size());

            // [# 1 ~ N] Column Data Fields : cc
            auto raws = compressed ? ctx.one_as_raws("orp:cols:comp")
                                   : ctx.n_raws("or:cols:n", get_cc(h.kdo.body));
            if (!raws) return tl::make_unexpected(raws.error());

            // [~]
            auto dscn = ctx.one_scn8_if("orp:dscn", h.is_rowDependencies());

            return Ch_Orp{
                std::move(h),
                std::move(*raws),
                dscn, compressed
            };
        }
    };

    struct Ch_Qmi : Ch_DoH {
        RawFlds row_raws;
        std::vector<uint16_t> row_sizes;

        explicit Ch_Qmi(Ch_DoH &&h, std::vector<uint16_t> &&sizes, RawFlds &&r) noexcept
            : Ch_DoH(std::move(h)), row_raws(std::move(r)), row_sizes(std::move(sizes)) {}

        static Result<Ch_Qmi> parse(SpanCursor &ctx, Ch_DoH &&h) {

            const auto nrow = get_nrow(h.kdo.body);

            // [# 1] row-sizes : nrow
            auto sizes = ctx.one_array<uint16_t>("qmi:r-sizes", nrow);
            if (!sizes) return tl::make_unexpected(sizes.error());

            // [# 2 ] rows
            auto raws = ctx.one_raws_by("qmi:rows:1", *sizes);

            return Ch_Qmi{
                std::move(h),
                std::move(*sizes),
                std::move(*raws)
            };
        }
    };

    // ================================================================================
    using Change_kdo = std::variant<Ch_Irp, // 11.2
                                    Ch_Drp, // 11.3
                                    Ch_Lrk, // 11.4
                                    Ch_Urp, // 11.5  +16 --> 11.21 log-miner support Urp2
                                    Ch_Orp, // 11.6
                                    Ch_Mfc, // 11.7
                                    Ch_Cfa, // 11.8
                                    Ch_Cki, // 11.9 Change Cluster Key Index
                                    Ch_Skl, // 11.10 Set Key link
                                    Ch_Qmi, // 11.11
                                    Ch_Qmd, // 11.12
                                    Ch_Dsc, // 11.14 Direct space cleanout
                                    Ch_Lmn, // 11.16
                                    Ch_Cmp, // 11.22

                                    Ch_Raw
    >;

    template<typename V, typename T>
    static Result<V> from(Result<Change_kdo> &&a) {
        if (!a) return tl::make_unexpected(a.error());

        if (std::holds_alternative<T>(*a))
            return V(std::get<T>(std::move(*a)));

        return err_of("not variant of Data-Op.");
    }

    // --------------------------------------------------------------------------------
    inline Result<Ch_DoH> Ch_DoH::parse(SpanCursor &ctx) {

        if (ctx.remaining() < 1) return err_of("Ch_DoH: empty span");

        Ch_DoH out;

        // [# 1] ktb
        if (ctx.has_nonEmpty_next()) {
            if (auto a = ctx.one_of<KtbVector>("DoH:ktb", KtbVector::decode)) out.ktb = std::move(*a);
            else return tl::make_unexpected(a.error());
        } else {
            // In some case (ex:11.8) ktb field is empty. --- allow ktb to be null-opt.
            ctx.skip_empty_span();
        }

        // [# 2] Kdo
        if (auto a = ctx.one_of<KdoVector>("DoH:kdo", KdoVector::decode)) out.kdo = std::move(*a);
        else return tl::make_unexpected(a.error());

        out.span_remain = ctx.remaining();

        return out;
    }

    inline Result<Change_kdo> parse_kdop(SpanCursor &ctx, const std::string_view name) {

        // [#1, 2] KTB ~ KDO
        auto hdr = Ch_DoH::parse(ctx);
        if (!hdr) return tl::make_unexpected(hdr.error());

        const auto type = get_kdoType(hdr->kdo.head.op);

        // todo g3nie --- check following spans
        switch (type) {
            case KdoType::Lkr: return Ch_Lrk{std::move(*hdr)};
            case KdoType::Qmd: return Ch_Qmd{std::move(*hdr)};
            case KdoType::Cki: return Ch_Cki{std::move(*hdr)};  // todo
            case KdoType::Skl: return Ch_Skl{std::move(*hdr)};  // todo
            case KdoType::Dsc: return Ch_Dsc{std::move(*hdr)};

            case KdoType::Mfc: { // p1 ~ p2
                if (auto o = Ch_Mfc::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Cfa: { // sup#
                if (auto o = Ch_Cfa::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Lmn: { // sup#
                if (auto o = Ch_Lmn::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Drp: { // dscn
                if (auto o = Ch_Drp::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Irp: { // cols#(cc)|1(cmp) ~ dscn
                if (auto o = Ch_Irp::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Urp: { // arr ~ cols#(nnew)|1(cmp) ~ dscn
                if (auto o = Ch_Urp::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Orp: { // cols#(cc)|1(cmp) ~ dscn
                if (auto o = Ch_Orp::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }
            case KdoType::Qmi: { // 2: arr(nrow) ~ 1(rows)
                if (auto o = Ch_Qmi::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Cmp: { // 2 : p1 ~ p2
                if (auto o = Ch_Cmp::parse(ctx, std::move(*hdr))) return *o;
                else return tl::make_unexpected(o.error());
            }

            default:
                return err_of(fmt::format("{}: Unknown Kdo.op: 0x{:x}", name, hdr->kdo.head.op));
        }
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ch_DoH &a) {
        return fmt::format("{}\n{}\n  --- span_remain#: {}",
            a.ktb ? to_string(*a.ktb) : "  KTB { Empty }",
            to_string(a.kdo), a.span_remain);
    }

    inline std::string to_string(const std::string_view prefix, const Ch_DoH &a) {
        return fmt::format("{}:\n{}\n{}\n  --- span_remain#: {}", prefix,
            a.ktb ? to_string(*a.ktb) : "  KTB { Empty }",
            to_string(a.kdo), a.span_remain);
    }

    static std::string to_string(const Ch_Lrk &a) { return to_string("Lrk:", static_cast<const Ch_DoH &>(a)); }
    static std::string to_string(const Ch_Qmd &a) { return to_string("Qmd:", static_cast<const Ch_DoH &>(a)); }
    static std::string to_string(const Ch_Cki &a) { return to_string("Cki:", static_cast<const Ch_DoH &>(a)); }
    static std::string to_string(const Ch_Skl &a) { return to_string("Skl:", static_cast<const Ch_DoH &>(a)); }
    static std::string to_string(const Ch_Dsc &a) { return to_string("Dsc:", static_cast<const Ch_DoH &>(a)); }
    static std::string to_string(const Ch_Raw &a) { return to_string("Raw:", static_cast<const Ch_DoH &>(a)); }

    static std::string to_string(const Ch_Cmp &a) {
        return fmt::format("Cmp:\n{}{}{}",
            to_string(static_cast<const Ch_DoH &>(a)),
            a.payload0 ? "\n  payload0" + to_string(*a.payload0) : "",
            a.payload1 ? "\n  payload1" + to_string(*a.payload1) : "" );
    }

    static std::string to_string(const Ch_Mfc &a) {
        return fmt::format("Mfc:\n{}{}{}",
            to_string(static_cast<const Ch_DoH &>(a)),
            a.payload0 ? "\n  payload0" + to_string(*a.payload0) : "",
            a.payload1 ? "\n  payload1" + to_string(*a.payload1) : "" );
    }
    static std::string to_string(const Ch_Cfa &a) {
        return fmt::format("Cfa:\n{}\n  {}",
            to_string(static_cast<const Ch_DoH &>(a)),
            a.spl ? to_string(*a.spl) : "SUP: Empty"
         );
    }
    static std::string to_string(const Ch_Lmn &a) {
        return fmt::format("Lmn:\n{}\n  {}",
            to_string(static_cast<const Ch_DoH &>(a)),
            a.spl ? to_string(*a.spl) : "SUP: Empty"
         );
    }
    static std::string to_string(const Ch_Drp &a) {
        return fmt::format("Drp:\n{} {}", to_string(static_cast<const Ch_DoH &>(a)),
                           a.dscn ? fmt::format("dscn: {}", *a.dscn) : ""
         );
    }

    static std::string to_string(const Ch_Irp &a) {
        return fmt::format("Irp:\n{} {}\n  cols#: {} {}\n{}",
                           to_string(static_cast<const Ch_DoH &>(a)),
                           a.dscn ? fmt::format("dscn: {}", *a.dscn) : "",
                           a.col_raws.size(),
                           a.compressed ? "compressed" : "",
                           to_string(a.col_raws)
                           );
    }
    static std::string to_string(const Ch_Urp &a) {
        return fmt::format("Urp:\n{} {}\n  cols#: {}\n{}",
            to_string(static_cast<const Ch_DoH &>(a)),
            a.dscn ? fmt::format("dscn: {}", *a.dscn) : "",
            a.col_indices.size(),
            to_string(a.col_raws));
    }
    static std::string to_string(const Ch_Orp &a) {
        return fmt::format("Orp:\n{} {}\n  cols#: {} {}\n{}",
            to_string(static_cast<const Ch_DoH &>(a)),
            a.dscn ? fmt::format("dscn: {}", *a.dscn) : "",
            a.col_raws.size(),
            a.compressed ? "compressed" : "",
            to_string(a.col_raws)
            );
    }
    static std::string to_string(const Ch_Qmi &a) {
        return fmt::format("Qmi:\n{}\n  rows#: {}\n{}",
            to_string(static_cast<const Ch_DoH &>(a)),
            a.row_sizes.size(),
            to_string(a.row_raws) );
    }

    inline std::string to_string(const Change_kdo &ckdo) {
        return std::visit([](const auto &c) { return to_string(c); }, ckdo);
    }
}
