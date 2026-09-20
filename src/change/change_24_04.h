#pragma once
#include <fmt/format.h>
#include <optional>
#include <variant>
#include "../coral_combinator.h"
#include "../coral_decode.h"
#include "../elements/layout_24.h"
#include "tl/expected.hpp"

namespace ora {

    using coral::decode_at, coral::Result, coral::err_of;
    using std::optional, std::nullopt, std::variant;

    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// Type 0x06 / 0x07 (Pseudo Transaction)
    struct PseudoTx {
        Ktptx ptxh;                     // # 2: Pseudo Transaction Header (28 Bytes)
    };

    /// Type 0x0E (Min Active Transaction)
    struct MinActiveTx {
        uint64_t min_act_tx_scn;         // # 2: MinActTxScn (8 Bytes SCN)
    };

    /// Type 0x0C (Transaction Finalized)
    struct FinalizedTx {
        Kttxs txs;                    // # 2: Transaction State Vector (4 Bytes)
        RawFld unknown;                 // # 3: Unknown / Padding (4 Bytes)
        optional<uint64_t> tx_start_scn;// # 4: TxStartScn (8 Bytes SCN)
    };

    using MrmBody = variant<            //
            PseudoTx,                   // Type 0x06 / 0x07 (Pseudo Transaction)
            FinalizedTx,                // Type 0x0C (Transaction Finalized)
            MinActiveTx,                // Type 0x0E (Min Active Transaction)
            RawFlds                     //
            >;

    // --------------------------------------------------------------------------------
    /// {24, 4, "KRVMISC", "Miscellaneous info"},
    struct Change_2404 {
        Ktmrm   rmh;     // # 1: Recovery Marker Header (16 Bytes, 필수)
        MrmBody body; // # 2~
    };

    // --------------------------------------------------------------------------------
    [[nodiscard]] inline Result<Change_2404> parse_2404( SpanCursor& ctx) {

        Change_2404 out;

        // [# 1] rmh (Media Recovery Marker)
        auto rmh = ctx.one<Ktmrm>("Ch24_4:mrm", [&](auto s) { return decode_ktmrm(s, ctx.isLittle); });
        if (!rmh) return tl::make_unexpected(rmh.error());
        out.rmh = *rmh;

        if (!ctx.has_remaining()) return out;

        // ----------------------------
        switch (out.rmh.type) {
            case 0x06:
            case 0x07: {
                // [# 2] Pseudo Transaction Marker
                auto o_ktptx = ctx.one<Ktptx>("Ch24_4:KTPTX", [&](auto s) { return decode_ktptx(s, ctx.isLittle); });
                if (!o_ktptx) return tl::make_unexpected(o_ktptx.error());
                out.body = PseudoTx{ .ptxh = *o_ktptx };
                return out;
            }

            case 0x0C: {
                FinalizedTx fin;
                // [# 2] Transaction State Vector
                auto o_kttxs = ctx.one<Kttxs>("Ch24_4:KTTXS", [&](auto s) { return decode_kkttxs(s, ctx.isLittle); });
                if (!o_kttxs) return tl::make_unexpected(o_kttxs.error());
                fin.txs = *o_kttxs;

                // [# 3] Unknown
                auto uk = ctx.raw("Ch24_4:Unk3");
                if (!uk) return tl::make_unexpected(uk.error());

                // [# 4] TxStartScn (8 Bytes SCN, Optional)
                auto o_scn = ctx.one_scn8("Ch24_4:TxStartScn", ctx.isLittle);
                if (o_scn) fin.tx_start_scn = *o_scn;

                out.body = fin;
                return out;
            }

            case 0x0E: {
                // [# 2] MinActTxScn (8 Bytes SCN)
                auto o_scn = ctx.one_scn8("Ch24_4:MinActTxScn", ctx.isLittle);
                if (!o_scn) return tl::make_unexpected(o_scn.error());

                out.body = MinActiveTx{ .min_act_tx_scn = *o_scn };
                return out;
            }

            default: {
                // [# 2~ ] Unknowns
                if (auto rs = ctx.rest("Ch24_4:unknown-type")) out.body = std::move(*rs);

                return out;
            }
        }
        return out;
    }

}