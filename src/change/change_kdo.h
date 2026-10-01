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

    /// Data Operation Header -- KTB ~ KDO
    struct Ch_DoH {
        KtbVector ktb; // # 1
        KdoVector kdo; // # 2

        bool is_kdom2() const noexcept { return kdo.head.is_kdom2(); }
        bool is_rowDependencies() const noexcept { return ora::is_rowDependencies(kdo.head.op); }
        bool isCompressed(size_t span_size) const { return ora::isCompressed(kdo.body, span_size); }

        static Result<Ch_DoH> parse(SpanCursor &ctx, std::string_view name);
    };

    // --------------------------------------------------------------------------------
    struct Ch_Lrk : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Lrk(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Mfc : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Mfc(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Cfa : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Cfa(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Qmd : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Qmd(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };
    struct Ch_Lmn : Ch_DoH { using Ch_DoH::Ch_DoH; explicit Ch_Lmn(Ch_DoH &&h) noexcept : Ch_DoH(std::move(h)) {} };

    // --------------------------------------------------------------------------------
    struct Ch_Drp : Ch_DoH {
        using Ch_DoH::Ch_DoH;
        std::optional<uint64_t> dscn; // row-dependency scn

        // ----------------------------------------
        explicit Ch_Drp(Ch_DoH &&h, std::optional<uint64_t>d) noexcept
        : Ch_DoH(std::move(h)), dscn{d} {}

        static Result<Ch_Drp> parse(SpanCursor &ctx, Ch_DoH &&h, std::string_view name) {
            return Ch_Drp{
                std::move(h),
                ctx.one_scn8_if(fmt::format("{}:Drp:scn", name), h.is_rowDependencies())};
        }
    };

    struct Ch_Irp : Ch_DoH {
        using Ch_DoH::Ch_DoH;
        RawFlds col_raws;
        std::optional<uint64_t> dscn;  // row-dependency scn
        bool compressed;

        // ----------------------------------------
        explicit Ch_Irp(Ch_DoH &&h, RawFlds &&r, std::optional<uint64_t>d, bool isCompressed = false) noexcept
            : Ch_DoH(std::move(h)), col_raws(std::move(r)), dscn{d}, compressed {isCompressed} {}

        static Result<Ch_Irp> parse(SpanCursor &ctx, Ch_DoH &&h, std::string_view name) {

            auto s3 = ctx.peek();
            if (!s3) return Ch_Irp{ std::move(h), RawFlds{}, std::nullopt };

            const auto compressed = h.isCompressed(s3->size());

            // [# 3 ~ N] Column Data Fields : cc
            auto raws = compressed ? ctx.one_as_raws("col_raws:compressed")
                                   : ctx.n_raws(fmt::format("{}:col_raws", name), get_cc(h.kdo.body));

            if (!raws) return tl::make_unexpected(raws.error());

            auto dscn = ctx.one_scn8_if(fmt::format("{}:dscn", name), h.is_rowDependencies());

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

        static Result<Ch_Urp> parse(SpanCursor &ctx, Ch_DoH &&h, std::string_view name) {

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

    struct Ch_Orp : Ch_DoH {
        RawFlds col_raws;
        std::optional<uint64_t> dscn; // row-dependency scn
        bool compressed;

        explicit Ch_Orp(Ch_DoH &&h, RawFlds &&r, std::optional<uint64_t> d, bool isCompressed = false) noexcept
            : Ch_DoH(std::move(h)), col_raws(std::move(r)), dscn{d}, compressed {isCompressed} {}

        static Result<Ch_Orp> parse(SpanCursor &ctx, Ch_DoH &&h, std::string_view name) {

            auto s3 = ctx.peek();
            if (!s3) return Ch_Orp{ std::move(h), RawFlds{}, std::nullopt };

            const auto compressed = h.isCompressed(s3->size());

            // [# 3 ~ N] Column Data Fields : cc
            auto raws = compressed ? ctx.one_as_raws("col_raws:compressed")
                                   : ctx.n_raws(fmt::format("{}:orp:col_raw", name), get_cc(h.kdo.body));
            if (!raws) return tl::make_unexpected(raws.error());

            // [~]
            auto dscn = ctx.one_scn8_if(fmt::format("{}:orp:row_dep", name), h.is_rowDependencies());

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

        static Result<Ch_Qmi> parse(SpanCursor &ctx, Ch_DoH &&h, std::string_view name) {

            const auto nrow = get_nrow(h.kdo.body);

            // [# 3] row-sizes : nrow
            auto sizes = ctx.one_array<uint16_t>(fmt::format("{}:qmi:row_sizes", name), nrow);
            if (!sizes) return tl::make_unexpected(sizes.error());

            // [# 4 ] rows
            auto raws = ctx.one_raws_by(fmt::format("{}:qmi:row_raws", name), *sizes);

            return Ch_Qmi{
                std::move(h),
                std::move(*sizes),
                std::move(*raws)
            };
        }
    };

    // ================================================================================
    using Change_kdo = std::variant <
        Ch_Irp, Ch_Drp,
        Ch_Lrk, Ch_Urp,
        Ch_Orp, Ch_Mfc,
        Ch_Cfa, Ch_Qmi,
        Ch_Qmd, Ch_Lmn
    >;

    template<typename V, typename T>
    static Result<V> from(Result<Change_kdo> &&a) {
        if (!a) return tl::make_unexpected(a.error());

        if (std::holds_alternative<T>(*a))
            return V(std::get<T>(std::move(*a)));

        return err_of("not variant of Data-Op.");
    }

    // --------------------------------------------------------------------------------
    inline Result<Ch_DoH> Ch_DoH::parse(SpanCursor &ctx, std::string_view name) {

        // [# 1] ktb
        auto ktb = ctx.one_of<KtbVector>(fmt::format("{}:ktb", name), decode_ktb);
        if (!ktb) return tl::make_unexpected(ktb.error());

        // [# 2] Kdo
        auto kdo = ctx.one_of<KdoVector>(fmt::format("{}:kdo", name), KdoVector::decode);
        if (!kdo) return tl::make_unexpected(kdo.error());

        return Ch_DoH {
            std::move(*ktb),
            std::move(*kdo)
        };
    }

    inline Result<Change_kdo> parse_kdop(SpanCursor &ctx, const std::string_view name) {

        // [#1, 2] KTB ~ KDO
        auto hdr = Ch_DoH::parse(ctx, name);
        if (!hdr) return tl::make_unexpected(hdr.error());

        const auto type = get_kdoType(hdr->kdo.head.op);

        // todo :: g3nie -- check missing
        // KdoCkiBody, // 0x09 (Change Cluster key Index)
        // KdoSklBody, // 0x0A (Set key link)
        // KdoDscBody, // 0x0E
        // KdoRawBody  // fallback

        switch (type) {
            case KdoType::Lkr: return Ch_Lrk{std::move(*hdr)};
            case KdoType::Mfc: return Ch_Mfc{std::move(*hdr)};
            case KdoType::Cfa: return Ch_Cfa{std::move(*hdr)};
            case KdoType::Qmd: return Ch_Qmd{std::move(*hdr)};
            case KdoType::Lmn: return Ch_Lmn{std::move(*hdr)};  // todo :: check following spans

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

            default: return err_of(fmt::format("{} : UnknownKdoType : {:x}", name, hdr->kdo.head.op));
        }
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Ch_DoH &a) {
        return fmt::format("{}\n{}", to_string(a.ktb), to_string(a.kdo));
    }

    static std::string to_string(const Ch_Lrk &a) { return fmt::format("Lrk:\n{}", to_string(static_cast<const Ch_DoH &>(a))); }
    static std::string to_string(const Ch_Mfc &a) { return fmt::format("Mfc:\n{}", to_string(static_cast<const Ch_DoH &>(a))); }
    static std::string to_string(const Ch_Cfa &a) { return fmt::format("Cfa:\n{}", to_string(static_cast<const Ch_DoH &>(a))); }
    static std::string to_string(const Ch_Qmd &a) { return fmt::format("Qmd:\n{}", to_string(static_cast<const Ch_DoH &>(a))); }
    static std::string to_string(const Ch_Lmn &a) { return fmt::format("Lmn:\n{}", to_string(static_cast<const Ch_DoH &>(a))); }
    static std::string to_string(const Ch_Drp &a) { return fmt::format("Drp:\n{}", to_string(static_cast<const Ch_DoH &>(a))); }

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
