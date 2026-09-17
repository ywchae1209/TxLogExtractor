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

    struct Ch_sup {
        Ktusp spl;
        std::vector<uint16_t> col_ids;
        std::vector<uint16_t> col_sizes;
        RawFlds col_raws;
    };

    inline Result<Ch_sup> parse_sup (SpanCursor &ctx, const std::string_view name, const bool isLittle) {

        // [#1] Ktusp
        auto usp = ctx.one<Ktusp>(name, [&](auto s) { return decode_ktusp(s, isLittle); });
        if (!usp) return tl::make_unexpected(usp.error());

        const auto col_cnt = usp->col_cnt;

        // [#2] col-indices
        auto col_ids = ctx.one_array<uint16_t>(name, col_cnt, isLittle);
        if (!col_ids) return tl::make_unexpected(usp.error());

        // [#3] col-sizes
        auto col_sizes = ctx.one_array<uint16_t>(name, col_cnt, isLittle);
        if (!col_sizes) return tl::make_unexpected(usp.error());

        // [#4~ N] col-raws
        auto col_raws = ctx.raws_by(name, *col_sizes);
        if (!col_raws) return tl::make_unexpected(usp.error());

        return Ch_sup{
            .spl = std::move(*usp),
            .col_ids = std::move(*col_ids),
            .col_sizes = std::move(*col_sizes),
            .col_raws = std::move(*col_raws)
        };
    }

    // ================================================================================
    struct Ch_hdr {
        KtbVector ktb;  // # 1: Transaction Layer Redo
        KdoVector kdo; // # 2: KDO Head + variant-body
    };

    [[nodiscard]] inline Result<Ch_hdr> parse_kdo_hdr(SpanCursor &ctx,
                                                   const std::string_view name1,
                                                   const std::string_view name2, const bool isLittle) {
        // [# 1] ktb
        auto ktb = ctx.one<KtbVector>(name1, [&](auto s) { return decode_ktb(s, isLittle); });
        if (!ktb) return tl::make_unexpected(ktb.error());

        // [# 2] Kdo
        auto kdo = ctx.one<KdoVector>(name2, [&](auto s) { return decode_kdo(s, isLittle); });
        if (!kdo) return tl::make_unexpected(kdo.error());

        return Ch_hdr {
            std::move(*ktb),
            std::move(*kdo)
        };
    }

    // ================================================================================
    struct Ch_Drp : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        explicit Ch_Drp(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {}
    };
    struct Ch_Lrk : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        explicit Ch_Lrk(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {}
    };
    struct Ch_Mfc : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        explicit Ch_Mfc(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {}
    };
    struct Ch_Cfa : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        explicit Ch_Cfa(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {}
    };
    struct Ch_Qmd : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        explicit Ch_Qmd(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {}
    };
    /// KdoPurge/DrpBody --- todo
    struct Ch_Pdr : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        explicit Ch_Pdr(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {}
    };

    /// Lmn
    struct Ch_Lmn : Ch_hdr {
        using Ch_hdr::Ch_hdr;
        explicit Ch_Lmn(Ch_hdr &&h) noexcept : Ch_hdr(std::move(h)) {}
    };
    struct Ch_Irp : Ch_hdr {
        RawFlds col_raws;
        explicit Ch_Irp(Ch_hdr &&h, RawFlds &&payloads) noexcept :
            Ch_hdr(std::move(h)), col_raws(std::move(payloads)) {}
    };
    struct Ch_Urp : Ch_hdr {
        std::vector<uint16_t> change_col_indices;
        RawFlds col_raws;
        explicit Ch_Urp(Ch_hdr &&h, std::vector<uint16_t> &&idx, RawFlds &&payloads) noexcept :
            Ch_hdr(std::move(h)), change_col_indices(std::move(idx)), col_raws(std::move(payloads)) {}
    };
    struct Ch_Orp : Ch_hdr {
        RawFlds col_raws;
        explicit Ch_Orp(Ch_hdr &&h, RawFlds &&payloads) noexcept :
            Ch_hdr(std::move(h)), col_raws(std::move(payloads)) {}
    };
    struct Ch_Qmi : Ch_hdr {
        std::vector<uint16_t> row_sizes;
        RawFlds row_raws;
        Ch_Qmi(Ch_hdr &&h, std::vector<uint16_t> &&sizes, RawFlds &&payloads) noexcept :
            Ch_hdr(std::move(h)), row_sizes(std::move(sizes)), row_raws(std::move(payloads)) {}
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
    [[nodiscard]] inline Result<Ch_ktdo> parse_kdo(SpanCursor &ctx,
        const std::string_view name,
        const bool isLittle) {

        // [#1, 2] KTB ~ KDO
        auto sp12 = parse_kdo_hdr(ctx, fmt::format("{}:ktb", name), fmt::format("{}:kdo", name), isLittle);
        if (sp12) return tl::make_unexpected(sp12.error());

        const auto type = sp12->kdo.head.get_type();

        switch (type) {
            case KdoType::Drp: return Ch_Drp{std::move(*sp12)};
            case KdoType::Lkr: return Ch_Lrk{std::move(*sp12)};
            case KdoType::Mfc: return Ch_Mfc{std::move(*sp12)};
            case KdoType::Cfa: return Ch_Cfa{std::move(*sp12)};
            case KdoType::Qmd: return Ch_Qmd{std::move(*sp12)};
            case KdoType::Lmn: return Ch_Lmn{std::move(*sp12)};     //

            case KdoType::Irp: {
                const auto col_cnt = get_cc(sp12->kdo.body);

                // [# 3 ~ N] Column Data Fields : cc
                auto raws = ctx.n_raws("Ch11_2:col_raw", col_cnt);
                if (!raws) return tl::make_unexpected(raws.error());

                return Ch_Irp{
                    std::move(*sp12),
                    std::move(*raws)
                };
            }

            case KdoType::Urp: {
                const auto nnew = get_nnew(sp12->kdo.body);

                // [# 3] Column Index Array : nnew
                auto indices = ctx.one_array<uint16_t>("Ch11_5:col_nums", nnew, isLittle);
                if (!indices) return tl::make_unexpected(indices.error());

                // [# 4 ~ N] updated col-data : nnew
                auto raws = ctx.n_raws("Ch11_5:col_raw", nnew);
                if (!raws) return tl::make_unexpected(raws.error());

                return Ch_Urp{
                    std::move(*sp12),
                    std::move(*indices),
                    std::move(*raws)};
            }

            case KdoType::Orp: {
                const auto col_cnt = get_cc(sp12->kdo.body);

                // [# 3 ~ N] Column Data Fields : cc
                auto raws = ctx.n_raws("Ch11_6:col_raw", col_cnt);
                if (!raws) return tl::make_unexpected(raws.error());

                return Ch_Orp{
                    std::move(*sp12),
                    std::move(*raws)};
            }

            case KdoType::Qmi: {

                const auto nrow = get_nrow(sp12->kdo.body);

                // [# 3] Row Size Table : nrow
                auto row_sizes = ctx.one_array<uint16_t>("Ch11_11:row_sizes", nrow, isLittle);
                if (!row_sizes) return tl::make_unexpected(row_sizes.error());

                // [# 4 ~ N] Multi-Row Data : nrow
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
