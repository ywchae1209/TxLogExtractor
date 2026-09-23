#pragma once

#include <string>
#include <fmt/format.h>
#include "../coral_combinator.h"
#include "../elements/layout_kdo.h"
#include "../elements/layout_ktb.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::Result, coral::err_of;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    struct Ch_hdr {
        KtbVector ktb; // # 1
        KdoVector kdo; // # 2: KDO Head + variant-body

        bool is_kdom2() const noexcept { return kdo.head.is_kdom2(); }
        bool is_rowDependencies() const noexcept { return ora::is_rowDependencies(kdo.head.op); }

        static Result<Ch_hdr> parse(SpanCursor &ctx, std::string_view name);
    };

    // --------------------------------------------------------------------------------
    struct Ch_Lrk : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Lrk(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };
    struct Ch_Mfc : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Mfc(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };
    struct Ch_Cfa : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Cfa(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };
    struct Ch_Qmd : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Qmd(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };
    struct Ch_Lmn : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Lmn(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };

    // --------------------------------------------------------------------------------
    struct Ch_Drp : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        std::optional<uint64_t> dscn; // row-dependency scn

        // ----------------------------------------
        explicit Ch_Drp(Ch_hdr &&h, std::optional<uint64_t>d) noexcept
        : Ch_hdr(std::move(h)), dscn{d} {}

        static Result<Ch_Drp> parse(SpanCursor &ctx, Ch_hdr &&h, std::string_view name) {
            return Ch_Drp{
                std::move(h),
                ctx.one_scn8_if(fmt::format("{}:Drp:scn", name), h.is_rowDependencies())};
        }
    };

    struct Ch_Irp : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        RawFlds col_raws;
        std::optional<uint64_t> dscn;  // row-dependency scn

        // ----------------------------------------
        explicit Ch_Irp(Ch_hdr &&h, RawFlds &&r, std::optional<uint64_t>d) noexcept
            : Ch_hdr(std::move(h)), col_raws(std::move(r)), dscn{d} {}

        static Result<Ch_Irp> parse(SpanCursor &ctx, Ch_hdr &&h, std::string_view name) {

            const auto col_cnt = get_cc(h.kdo.body);

            // [# 3 ~ N] Column Data Fields : cc
            auto raws = ctx.n_raws(fmt::format("{}:col_raw", name), col_cnt);
            if (!raws) return tl::make_unexpected(raws.error());

            auto dscn = ctx.one_scn8_if(fmt::format("{}:dscn", name), h.is_rowDependencies());
            return Ch_Irp{
                std::move(h),
                std::move(*raws),
                dscn };
        }

    };

    struct Ch_Urp : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        RawFlds col_raws;
        std::vector<uint16_t> col_indices; // colVector | colElements
        std::optional<uint64_t> dscn;      // row-dependency scn

        explicit Ch_Urp(Ch_hdr &&h, std::vector<uint16_t> &&idx, RawFlds &&r, std::optional<uint64_t> d) noexcept
            : Ch_hdr(std::move(h)), col_raws(std::move(r)), col_indices(std::move(idx)), dscn{d} {}

        static Result<Ch_Urp> parse(SpanCursor &ctx, Ch_hdr &&h, std::string_view name) {

            const auto nnew = get_nnew(h.kdo.body);

            // [# 3] Column Index Array : nnew
            auto indices = ctx.one_array<uint16_t>(fmt::format("{}:urp:col_nums", name), nnew);
            if (!indices) return tl::make_unexpected(indices.error());

            // [#4 ]
            auto raws = h.is_kdom2()
                        ? ctx.one_raws_by(fmt::format("{}:urp:col-vector", name), *indices) // [# 4]   Col-Vector
                        : ctx.n_raws(fmt::format("{}:urp:col_raw", name), nnew);            // [# 4 ~] Col-Elements

            if (!raws) return tl::make_unexpected(raws.error());

            // [~]
            auto dscn = ctx.one_scn8_if(fmt::format("{}:urp:row_dep", name), h.is_rowDependencies());

            return Ch_Urp{
                std::move(h),
                std::move(*indices),
                std::move(*raws),
                dscn
            };
        }
    };

    struct Ch_Orp : Ch_hdr {
        RawFlds col_raws;
        std::optional<uint64_t> dscn; // row-dependency scn

        explicit Ch_Orp(Ch_hdr &&h, RawFlds &&r, std::optional<uint64_t> d) noexcept
            : Ch_hdr(std::move(h)), col_raws(std::move(r)), dscn{d} {}

        static Result<Ch_Orp> parse(SpanCursor &ctx, Ch_hdr &&h, std::string_view name) {

            const auto col_cnt = get_cc(h.kdo.body);

            // [# 3 ~ N] Column Data Fields : cc
            auto raws = ctx.n_raws(fmt::format("{}:orp:col_raw", name), col_cnt);
            if (!raws) return tl::make_unexpected(raws.error());

            // [~]
            auto dscn = ctx.one_scn8_if(fmt::format("{}:orp:row_dep", name), h.is_rowDependencies());

            return Ch_Orp{
                std::move(h),
                std::move(*raws),
                dscn
            };
        }
    };

    struct Ch_Qmi : Ch_hdr {
        RawFlds row_raws;
        std::vector<uint16_t> row_sizes;

        explicit Ch_Qmi(Ch_hdr &&h, std::vector<uint16_t> &&sizes, RawFlds &&r) noexcept
            : Ch_hdr(std::move(h)), row_raws(std::move(r)), row_sizes(std::move(sizes)) {}

        static Result<Ch_Qmi> parse(SpanCursor &ctx, Ch_hdr &&h, std::string_view name) {

            const auto nrow = get_nrow(h.kdo.body);

            // [# 3] Row Size Table : nrow
            auto row_sizes = ctx.one_array<uint16_t>(fmt::format("{}:qmi:row_sizes", name), nrow);
            if (!row_sizes) return tl::make_unexpected(row_sizes.error());

            // [# 4 ~ N] Multi-Row Data : nrow :: todo ::: dumpRows
            auto raws = ctx.raws_by(fmt::format("{}:qmi:row_raws", name), *row_sizes);
            if (!raws) return tl::make_unexpected(raws.error());

            return Ch_Qmi{
                std::move(h),
                std::move(*row_sizes),
                std::move(*raws)
            };
        }
    };

    // ================================================================================
    using Ch_ktdo = std::variant <
        Ch_Irp, Ch_Drp,
        Ch_Lrk, Ch_Urp,
        Ch_Orp, Ch_Mfc,
        Ch_Cfa, Ch_Qmi,
        Ch_Qmd, Ch_Lmn
    >;

    template<typename V, typename T>
    static Result<V> from(Result<Ch_ktdo> &&a) {
        if (!a) return tl::make_unexpected(a.error());

        if (std::holds_alternative<T>(*a))
            return V(std::get<T>(std::move(*a)));

        return err_of("not variant of Data-Op.");
    }

    // --------------------------------------------------------------------------------
    inline Result<Ch_hdr> Ch_hdr::parse(SpanCursor &ctx, std::string_view name) {

        // [# 1] ktb
        auto ktb = ctx.one_of<KtbVector>(fmt::format("{}:ktb", name), decode_ktb);
        if (!ktb) return tl::make_unexpected(ktb.error());

        // [# 2] Kdo
        auto kdo = ctx.one_of<KdoVector>(fmt::format("{}:kdo", name), decode_kdo);
        if (!kdo) return tl::make_unexpected(kdo.error());

        return Ch_hdr {
            std::move(*ktb),
            std::move(*kdo)
        };
    }

    /// Ktb ~ Kdo ~ KdoBody
    [[nodiscard]] inline Result<Ch_ktdo> parse_kdop(SpanCursor &ctx, const std::string_view name, const bool isLittle) {

        // [#1, 2] KTB ~ KDO
        auto hdr = Ch_hdr::parse(ctx, name);
        if (hdr) return tl::make_unexpected(hdr.error());

        const auto type = get_kdoType(hdr->kdo.head.op);

        switch (type) {
            case KdoType::Lkr: return Ch_Lrk{std::move(*hdr)};
            case KdoType::Mfc: return Ch_Mfc{std::move(*hdr)};
            case KdoType::Cfa: return Ch_Cfa{std::move(*hdr)};
            case KdoType::Qmd: return Ch_Qmd{std::move(*hdr)};
            case KdoType::Lmn: return Ch_Lmn{std::move(*hdr)};  // todo ::;

            case KdoType::Drp: {
                if (auto o = Ch_Drp::parse(ctx, std::move(*hdr), name)) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Irp: {
                if (auto o = Ch_Irp::parse(ctx, std::move(*hdr), name)) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Urp: {
                if (auto o = Ch_Urp::parse(ctx, std::move(*hdr), name)) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Orp: {
                if (auto o = Ch_Orp::parse(ctx, std::move(*hdr), name)) return *o;
                else return tl::make_unexpected(o.error());
            }

            case KdoType::Qmi: {
                if (auto o = Ch_Qmi::parse(ctx, std::move(*hdr), name)) return *o;
                else return tl::make_unexpected(o.error());
            }

            default: return err_of("UnknownKdoType");
        }
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ch_hdr &a) {
        return fmt::format("{}\n{}", to_string(a.ktb), to_string(a.kdo));
    }

    static std::string to_string(const Ch_Lrk &a) { return fmt::format("Lrk: {}", to_string(static_cast<const Ch_hdr &>(a))); }
    static std::string to_string(const Ch_Mfc &a) { return fmt::format("Mfc: {}", to_string(static_cast<const Ch_hdr &>(a))); }
    static std::string to_string(const Ch_Cfa &a) { return fmt::format("Cfa: {}", to_string(static_cast<const Ch_hdr &>(a))); }
    static std::string to_string(const Ch_Qmd &a) { return fmt::format("Qmd: {}", to_string(static_cast<const Ch_hdr &>(a))); }
    static std::string to_string(const Ch_Lmn &a) { return fmt::format("Lmn: {}", to_string(static_cast<const Ch_hdr &>(a))); }

    static std::string to_string(const Ch_Drp &a) {
        return fmt::format("Drp: {}", to_string(static_cast<const Ch_hdr &>(a)));
    }

    static std::string to_string(const Ch_Irp &a) {
        // todo have more
        return fmt::format("Irp: {} {}\n{}",
                           to_string(static_cast<const Ch_hdr &>(a)),
                           a.dscn ? fmt::format("dscn: {}", *a.dscn) : "",
                           to_string(a.col_raws));
    }
    static std::string to_string(const Ch_Urp &a) {
        // todo have more
        return fmt::format("Urp: {} {}",
            to_string(static_cast<const Ch_hdr &>(a)),
            a.dscn ? fmt::format("dscn: {}", *a.dscn) : ""
            );
    }
    static std::string to_string(const Ch_Orp &a) {
        // todo have more
        return fmt::format("Orp: {} {}",
            to_string(static_cast<const Ch_hdr &>(a)),
            a.dscn ? fmt::format("dscn: {}", *a.dscn) : ""
            );
    }
    static std::string to_string(const Ch_Qmi &a) {
        // todo have more
        return fmt::format("Qmi: {}",
            to_string(static_cast<const Ch_hdr &>(a))
            );
    }
}
