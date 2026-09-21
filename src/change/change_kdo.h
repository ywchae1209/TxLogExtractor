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

    // ================================================================================
    struct Ch_hdr {
        KtbVector ktb; // # 1: Transaction Layer Redo
        KdoVector kdo; // # 2: KDO Head + variant-body

        bool is_kdom2() const noexcept { return kdo.head.is_kdom2(); }
        bool is_rowDependencies() const noexcept { return ora::is_rowDependencies(kdo.head.op); }
    };

    [[nodiscard]] inline Result<Ch_hdr> parse_ktdo_hdr(SpanCursor &ctx,
                                                   const std::string_view name1,
                                                   const std::string_view name2, const bool isLittle) {
        // [# 1] ktb
        auto ktb = ctx.one_of<KtbVector>(name1, decode_ktb);
        if (!ktb) return tl::make_unexpected(ktb.error());

        // [# 2] Kdo
        auto kdo = ctx.one_of<KdoVector>(name2, decode_kdo);
        if (!kdo) return tl::make_unexpected(kdo.error());

        return Ch_hdr {
            std::move(*ktb),
            std::move(*kdo)
        };
    }

    // ================================================================================
    struct Ch_Lrk : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Lrk(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };
    struct Ch_Mfc : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Mfc(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };
    struct Ch_Cfa : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Cfa(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };
    struct Ch_Qmd : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Qmd(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };

    /// KdoPurge/DrpBody --- todo
    struct Ch_Pdr : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Pdr(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };

    /// Lmn
    struct Ch_Lmn : Ch_hdr { using Ch_hdr::Ch_hdr; explicit Ch_Lmn(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {} };

    struct Ch_Drp : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        std::optional<uint64_t> dscn; // row-dependency scn
        explicit Ch_Drp(Ch_hdr &&h, std::optional<uint64_t>d) noexcept
        : Ch_hdr(std::move(h)), dscn{d} {}
    };

    struct Ch_Irp : Ch_hdr {
        RawFlds col_raws;
        std::optional<uint64_t> dscn;  // row-dependency scn
        explicit Ch_Irp(Ch_hdr &&h, RawFlds &&payloads, std::optional<uint64_t>d) noexcept
            : Ch_hdr(std::move(h)), col_raws(std::move(payloads)), dscn{d} {}
    };
    struct Ch_Urp : Ch_hdr {
        RawFlds col_raws;
        std::vector<uint16_t> col_indices; // colVector | colElements
        std::optional<uint64_t> dscn;      // row-dependency scn
        explicit Ch_Urp(Ch_hdr &&h, std::vector<uint16_t> &&idx, RawFlds &&payloads, std::optional<uint64_t> d) noexcept
            : Ch_hdr(std::move(h)), col_raws(std::move(payloads)), col_indices(std::move(idx)), dscn{d} {}
    };
    struct Ch_Orp : Ch_hdr {
        RawFlds col_raws;
        std::optional<uint64_t> dscn; // row-dependency scn
        explicit Ch_Orp(Ch_hdr &&h, RawFlds &&payloads, std::optional<uint64_t> d) noexcept
            : Ch_hdr(std::move(h)), col_raws(std::move(payloads)), dscn{d} {}
    };
    struct Ch_Qmi : Ch_hdr {
        RawFlds row_raws;
        std::vector<uint16_t> row_sizes;
        Ch_Qmi(Ch_hdr &&h, std::vector<uint16_t> &&sizes, RawFlds &&payloads) noexcept
            : Ch_hdr(std::move(h)), row_raws(std::move(payloads)), row_sizes(std::move(sizes)) {}
    };

    // ================================================================================
    using Ch_ktdo = std::variant <
        Ch_Irp,
        Ch_Drp,
        Ch_Lrk,
        Ch_Urp,
        Ch_Orp,
        Ch_Mfc,
        Ch_Cfa,
        Ch_Qmi,
        Ch_Qmd,
        Ch_Lmn,
        Ch_Pdr
    >;

    // --------------------------------------------------------------------------------
    /// Ktb ~ Kdo ~ KdoBody
    [[nodiscard]] inline Result<Ch_ktdo> parse_ktdo(SpanCursor &ctx, const std::string_view name, const bool isLittle) {

        // [#1, 2] KTB ~ KDO
        auto sp12 = parse_ktdo_hdr(ctx, fmt::format("{}:ktb", name), fmt::format("{}:kdo", name), isLittle);
        if (sp12) return tl::make_unexpected(sp12.error());

        const auto type = get_kdoType(sp12->kdo.head.op);

        switch (type) {
            case KdoType::Lkr: return Ch_Lrk{std::move(*sp12)};
            case KdoType::Mfc: return Ch_Mfc{std::move(*sp12)};
            case KdoType::Cfa: return Ch_Cfa{std::move(*sp12)};
            case KdoType::Qmd: return Ch_Qmd{std::move(*sp12)};
            case KdoType::Lmn: return Ch_Lmn{std::move(*sp12)};     //

            case KdoType::Drp: {
                auto dscn = ctx.one_scn8_if("Ch11_2:dscn", isLittle, sp12->is_rowDependencies());
                return Ch_Drp{
                    std::move(*sp12),
                    dscn };
            }


            case KdoType::Irp: {
                const auto col_cnt = get_cc(sp12->kdo.body);

                // [# 3 ~ N] Column Data Fields : cc
                auto raws = ctx.n_raws("Ch11_2:col_raw", col_cnt);
                if (!raws) return tl::make_unexpected(raws.error());

                auto dscn = ctx.one_scn8_if("Ch11_2:dscn", isLittle, sp12->is_rowDependencies());
                return Ch_Irp{
                    std::move(*sp12),
                    std::move(*raws),
                    dscn };
            }

            case KdoType::Urp: {
                const auto nnew = get_nnew(sp12->kdo.body);

                // [# 3] Column Index Array : nnew
                auto indices = ctx.one_array<uint16_t>("Ch11_5:col_nums", nnew, isLittle);
                if (!indices) return tl::make_unexpected(indices.error());

                // [#4 ]
                auto raws = sp12->is_kdom2()                                     //
                                ? ctx.one_raws_by("Ch11_5:col-vector", *indices) // [# 4]   Col-Vector :: dumpColVector
                                : ctx.n_raws("Ch11_5:col_raw", nnew);            // [# 4 ~] Col-Elements : nnew

                if (!raws) return tl::make_unexpected(raws.error());

                // [~]
                auto dscn = ctx.one_scn8_if("Ch11_5:row_dep", isLittle, sp12->is_rowDependencies());

                return Ch_Urp{
                    std::move(*sp12),
                    std::move(*indices),
                    std::move(*raws),
                    dscn
                };

            }

            case KdoType::Orp: {
                const auto col_cnt = get_cc(sp12->kdo.body);

                // [# 3 ~ N] Column Data Fields : cc
                auto raws = ctx.n_raws("Ch11_6:col_raw", col_cnt);
                if (!raws) return tl::make_unexpected(raws.error());

                // [~]
                auto dscn = ctx.one_scn8_if("Ch11_5:row_dep", isLittle, sp12->is_rowDependencies());

                return Ch_Orp{
                    std::move(*sp12),
                    std::move(*raws),
                    dscn
                };

            }

            case KdoType::Qmi: {
                const auto nrow = get_nrow(sp12->kdo.body);

                // [# 3] Row Size Table : nrow
                auto row_sizes = ctx.one_array<uint16_t>("Ch11_11:row_sizes", nrow, isLittle);
                if (!row_sizes) return tl::make_unexpected(row_sizes.error());

                // [# 4 ~ N] Multi-Row Data : nrow :: todo ::: dumpRows
                auto raws = ctx.raws_by("Ch11_11:row_raws", *row_sizes);
                if (!raws) return tl::make_unexpected(raws.error());

                return Ch_Qmi{
                    std::move(*sp12),
                    std::move(*row_sizes),
                    std::move(*raws)
                };
            }

            default: return err_of("UnknownKdoType");
        }
    }
}
