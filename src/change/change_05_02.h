
#pragma once
#include <fmt/format.h>
#include <optional>
#include "../coral_combinator.h"
#include "tcb/span.hpp"
#include "tl/expected.hpp"

namespace ora {
    using coral::Result, coral::decode_At;
    using std::optional;
    using namespace combinator;

    // --------------------------------------------------------------------------------
    /// 5.2 #1 * KTUDH (KTU Undo Header)
    /// - https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-Ktudh
    struct Ktudh {
        uint16_t xid_slt;  //  XID slot
        uint32_t xid_sqn;  // Xid sqn
        uint32_t uba_dba;  //  Undo Block Address: DBA
        uint16_t uba_sqn;  //  Undo Block Address: Sequence
        uint8_t  uba_rec;  //  Undo Block Address: Record#
        uint16_t flg;      //  Transaction Flag
        uint16_t siz;      //  Size
        uint16_t fbi;      //  FBI
        uint16_t pxid_usn; //  Parent XID usn (when Distributed Tx)
        uint16_t pxid_slt; //  Parent XID slt
        uint32_t pxid_sqn; //  Parent XID sqn

        static Result<Ktudh> decode(tcb::span<const char> buf, bool isLittle) ;
    };

    // --------------------------------------------------------------------------------
    /// 5.2 #2 KTEOP (Extent Map Redo)
    struct Kteop {
        uint32_t ext;       // Offset  4 ~ 7  : Extent Number (ext#)
        uint32_t ext_size;  // Offset 12 ~ 15 : Extent Size (ext size)
        uint32_t highWater; // Offset 16 ~ 19 : High Water Mark (SETHWM)
        uint32_t offset;    // Offset 24 ~ 27 : Map Offset

        uint32_t blk = 0;             // todo :: pos // Block Number (blk#)
        uint32_t blocks_freelist = 0; // todo :: pos //#blocks in seg. hdr's freelists
        uint32_t blocks_below = 0;    // todo :: pos //#blocks below
        uint32_t mapblk = 0;          // todo :: pos //Map Block Address

        static Result<Kteop> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// 5.2 #2 or #3 PDB Information (4 Bytes)
    struct PdbInfo {
        uint32_t pdbid; // PDB DB id

        static Result<PdbInfo> decode(tcb::span<const char> buf, bool isLittle);
    };

    // --------------------------------------------------------------------------------
    inline Result<Ktudh> Ktudh::decode(tcb::span<const char> buf, bool isLittle) {
        if ( buf.size() < 32) {
            return err_of(fmt::format("[Ktudh] buf ({}) < {}", buf.size(), 32));
        }

        return Ktudh{
            .xid_slt  = decode_At<uint16_t>(buf, isLittle, 0),
            .xid_sqn  = decode_At<uint32_t>(buf, isLittle, 4),
            .uba_dba  = decode_At<uint32_t>(buf, isLittle, 8),
            .uba_sqn  = decode_At<uint16_t>(buf, isLittle, 12),
            .uba_rec  = decode_At<uint8_t >(buf, isLittle, 14),
            .flg      = decode_At<uint16_t>(buf, isLittle, 16),
            .siz      = decode_At<uint16_t>(buf, isLittle, 18),
            .fbi      = decode_At<uint16_t>(buf, isLittle, 20),
            .pxid_usn = decode_At<uint16_t>(buf, isLittle, 24),
            .pxid_slt = decode_At<uint16_t>(buf, isLittle, 26),
            .pxid_sqn = decode_At<uint32_t>(buf, isLittle, 28)
        };
    }

    /// {5, 2, "KTURDH", "Update rollback segment header"},
    struct Change_0502 {
        Ktudh             udh; // #1  : Undo Header
        optional<Kteop>   eop; // #2  : Extent Map Redo (optional)
        optional<PdbInfo> pdb; // #2|3: PDB Info

        static Result<Change_0502> parse(SpanCursor &ctx);
    };

    // --------------------------------------------------------------------------------
    inline Result<PdbInfo> PdbInfo::decode(tcb::span<const char> buf, bool isLittle) {
        if (auto check = enough(buf, 4, "PDB"); !check) {
            return tl::make_unexpected(check.error());
        }

        return PdbInfo{
            .pdbid = decode_At<uint32_t>(buf, isLittle, 0)
        };
    }

    inline Result<Kteop> Kteop::decode(tcb::span<const char> buf, bool isLittle) {
        if (auto check = enough(buf, 36, "kteop"); !check) {
            return tl::make_unexpected(check.error());
        }

        return Kteop{
            .ext        = decode_At<uint32_t>(buf, isLittle, 4),
            .ext_size   = decode_At<uint32_t>(buf, isLittle, 12),
            .highWater  = decode_At<uint32_t>(buf, isLittle, 16),
            .offset     = decode_At<uint32_t>(buf, isLittle, 24)
        };
    }

    inline Result<Change_0502> Change_0502::parse( SpanCursor &ctx ) {

        Change_0502 out;

        // [# 1] udh
        auto udh = ctx.one_of<Ktudh>("Ch5_2:udh", Ktudh::decode);
        if (!udh) return tl::make_unexpected(udh.error());
        out.udh = std::move(*udh);

        // [# 2,3] (eop?) ~ pdb
        if (const auto span2 = ctx.next("Ch5_2:pdb|eop"); span2) {

            if (span2->size() >= 36) {
                auto eop2 = Kteop::decode(*span2, ctx.isLittle);
                if (!eop2) return tl::make_unexpected(eop2.error());
                out.eop = std::move(*eop2);
            }

            // [# 2 | 3] if left, Pdb
            auto pdb3 = ctx.one_of<PdbInfo>("Ch5_2:pdb", PdbInfo::decode);
            if (!pdb3) return out;
            out.pdb = std::move(*pdb3);
        }

        return out;
    }

    // --------------------------------------------------------------------------------
    static std::string to_string(const Kteop &a) {
        return fmt::format(
            "kteop redo - redo operation on extent map\n"
            "   SETHWM:       Highwater:: 0x{:08x} ext#: {:<6} blk#: {:<6} ext size: {:<6}\n"
            "  #blocks in seg. hdr's freelists: {}\n"
            "  #blocks below: {:<6}\n"
            "  mapblk  0x{:08x} offset: {:<6}\n",
            a.highWater, a.ext, a.blk, a.ext_size,
            a.blocks_freelist,
            a.blocks_below,
            a.mapblk, a.offset
        );
    }

    static std::string to_string(const Ktudh& a) {
        return fmt::format("Ktudh : "
                           "slt: 0x{:x} sqn:{} flg:{}, siz:{} fbi:{} "
                           "uba: 0x{:08x}.{:04x}.{:02x} pxid: 0x{:x}.{:x}.{:x}]",
                           a.xid_slt, a.xid_sqn, a.flg, a.siz, a.fbi,
                           a.uba_dba, a.uba_sqn, a.uba_rec,
                           a.pxid_usn, a.pxid_slt, a.pxid_sqn);
    }

    static std::string to_string(const PdbInfo& a) {
        return fmt::format("pdbid: {}", a.pdbid);
    }

    static std::string to_string(const Change_0502& a) {
        return fmt::format("Ch 5.2: {}\n"
                           "        {} {}",
            to_string(a.udh),
            a.eop ? to_string(*a.eop) : "",
            a.pdb ? to_string(*a.pdb) : "");
    }

}