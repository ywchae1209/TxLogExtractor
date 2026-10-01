#pragma once

#include <variant>
#include "../coral_decode.h"
#include "../coral_result.h"
#include "tcb/span.hpp"

////
///https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-KDOvector(KernalDataOperation)
namespace ora {

    using coral::decode_at, coral::decode_At, coral::Result, coral::err_of;
    using std::optional;

    namespace FBFlag {
        constexpr uint8_t FB_N{0x01}; // 1 << 0
        constexpr uint8_t FB_P{0x02}; // 1 << 1
        constexpr uint8_t FB_L{0x04}; // 1 << 2
        constexpr uint8_t FB_F{0x08}; // 1 << 3
        constexpr uint8_t FB_D{0x10}; // 1 << 4
        constexpr uint8_t FB_H{0x20}; // 1 << 5
        constexpr uint8_t FB_C{0x40}; // 1 << 6
        constexpr uint8_t FB_K{0x80}; // 1 << 7
    }

    inline bool off_FB_L(uint8_t fb) { return (FBFlag::FB_L & fb) == 0; }
    inline bool on_FB_C(uint8_t fb) { return (FBFlag::FB_C & fb) != 0; }

    static std::string FB_string(uint8_t fb) {

        char s[9]{};

        s[7] = (fb & FBFlag::FB_N) ? 'N' : '-'; // The last column continues in the Next piece
        s[6] = (fb & FBFlag::FB_P) ? 'P' : '-'; // The first column continues from the Previous piece
        s[5] = (fb & FBFlag::FB_L) ? 'L' : '-'; // Last ctx piece
        s[4] = (fb & FBFlag::FB_F) ? 'F' : '-'; // First ctx piece
        s[3] = (fb & FBFlag::FB_D) ? 'D' : '-'; // Deleted row
        s[2] = (fb & FBFlag::FB_H) ? 'H' : '-'; // Head piece of row
        s[1] = (fb & FBFlag::FB_C) ? 'C' : '-'; // Clustered table member
        s[0] = (fb & FBFlag::FB_K) ? 'K' : '-'; // Cluster Key

        s[8] = '\0';

        return std::string(s);
    }

    //--------------------------------------------------------------------------------
    enum class KdoType : uint8_t {
        Iur = 0x01, //----  Interpret Undo Redo
        Irp = 0x02, // 0x23) Single Insert
        Drp = 0x03, // 0x22) Single Delete
        Lkr = 0x04, // Lock Row
        Urp = 0x05, // Single Update
        Orp = 0x06, // Overwrite Row
        Mfc = 0x07, // Manipulate First Column
        Cfa = 0x08, // Change Forwarding Address
        Cki = 0x09, // Change Cluster key Index
        Skl = 0x0A, // Set Key Links
        Qmi = 0x0B, // Quick Multi-Insert
        Qmd = 0x0C, // Quick Multi-Delete
        Dsc = 0x0e, // todo ::
        Lmn = 0x10, // Logmine r
        LLB = 0x11, // todo ::
        o19 = 0x13, // todo ::
        Shk = 0x14, // todo ::
        o21 = 0x15, // todo ::
        Cmp = 0x16, // todo ::
        Dcu = 0x17, // todo ::
        Mrk = 0x18, // todo ::
        Unknown = 0xFF,
    };

    inline KdoType get_kdoType(uint8_t op_code) {
        switch (static_cast<KdoType>(op_code & 0x1F)) {    // op_code & 0x1F or & 0x3F
            case KdoType::Iur: return KdoType::Iur; // todo :: check 0x21
            case KdoType::Irp: return KdoType::Irp; // Single Insert (Redo: 0x02, Undo: 0x23)
            case KdoType::Drp: return KdoType::Drp; // Single Delete (Redo: 0x03, Undo: 0x22)
            case KdoType::Lkr: return KdoType::Lkr;
            case KdoType::Urp: return KdoType::Urp;
            case KdoType::Orp: return KdoType::Orp;
            case KdoType::Mfc: return KdoType::Mfc;
            case KdoType::Cfa: return KdoType::Cfa;
            case KdoType::Cki: return KdoType::Cki; // todo
            case KdoType::Skl: return KdoType::Skl; // todo ---
            case KdoType::Qmi: return KdoType::Qmi;
            case KdoType::Qmd: return KdoType::Qmd;
            case KdoType::Dsc: return KdoType::Dsc; // todo
            case KdoType::Lmn: return KdoType::Lmn; // Logminer (0x10, 0x30)
            case KdoType::LLB: return KdoType::LLB; // todo
            case KdoType::o19: return KdoType::o19; // todo
            case KdoType::Shk: return KdoType::Shk; // todo
            case KdoType::o21: return KdoType::o21; // todo
            case KdoType::Cmp: return KdoType::Cmp; // todo
            case KdoType::Dcu: return KdoType::Dcu; // todo
            case KdoType::Mrk: return KdoType::Mrk; // todo
            default: return KdoType::Unknown;
        }
    }

    constexpr bool is_redo(uint8_t op) { return op < 0x20; }
    constexpr bool is_undo(uint8_t op) { return !is_redo(op); }

    constexpr uint8_t OP_ROWDEPENDENCIES{0x40};
    constexpr bool is_rowDependencies(uint8_t op) { return (op & OP_ROWDEPENDENCIES) != 0; }

    constexpr std::string_view kdoType_string(uint8_t op) {

        switch (static_cast<KdoType>(op & 0x1F)) {
            case KdoType::Iur: return "Iur";
            case KdoType::Irp: return "Irp"; // Single Insert (Redo: 0x02, Undo: 0x23)
            case KdoType::Drp: return "Drp"; // Single Delete (Redo: 0x03, Undo: 0x22)
            case KdoType::Lkr: return "Lkr";
            case KdoType::Urp: return "Urp";
            case KdoType::Orp: return "Orp";
            case KdoType::Mfc: return "Mfc";
            case KdoType::Cfa: return "Cfa";
            case KdoType::Cki: return "Cki";
            case KdoType::Skl: return "Skl";
            case KdoType::Qmi: return "Qmi";
            case KdoType::Qmd: return "Qmd";
            case KdoType::Dsc: return "Dsc";
            case KdoType::Lmn: return "Lmn";
            case KdoType::LLB: return "LLB";
            case KdoType::o19: return "o19";
            case KdoType::Shk: return "Shk";
            case KdoType::o21: return "o21";
            case KdoType::Cmp: return "Cmp";
            case KdoType::Dcu: return "Dcu";
            case KdoType::Mrk: return "Mrk";
            default: return "Unknown";
        }
    }

    //--------------------------------------------------------------------------------
    namespace KdoXAType {
        constexpr uint8_t FLAGS_XA{0x01};
        constexpr uint8_t FLAGS_XR{0x02};
        constexpr uint8_t FLAGS_CR{0x03};
        constexpr uint8_t FLAGS_KDO_KDOM2{0x80};
    }

    /// kdom2 ???
    constexpr bool is_kdom2(const uint8_t xt) { return (xt & KdoXAType::FLAGS_KDO_KDOM2) != 0; }
    constexpr bool is_xa(const uint8_t xt) { return (xt & 0x03) == KdoXAType::FLAGS_XA; }
    constexpr bool is_xr(const uint8_t xt) { return (xt & 0x03) == KdoXAType::FLAGS_XR; }
    constexpr bool is_cr(const uint8_t xt) { return (xt & 0x03) == KdoXAType::FLAGS_CR; }

    //--------------------------------------------------------------------------------
    /// 1-based index :: nullsDelta --> ccData :: bit 0 == not-null, 1 == null
    inline std::optional<uint16_t> last_on_index(tcb::span<const char> buf, uint16_t cc) {

        const size_t required_bytes = (static_cast<size_t>(cc) + 7U) / 8U;
        if (buf.size() < required_bytes) {
            return std::nullopt;
        }

        const auto* nulls = reinterpret_cast<const uint8_t*>(buf.data());

        // reverse-search
        for (size_t i = cc; i > 0; --i) {
            const size_t bit_idx = i - 1;
            const uint8_t byte_val = nulls[bit_idx / 8];
            const uint8_t bit_mask = static_cast<uint8_t>(1U << (bit_idx % 8));

            if ((byte_val & bit_mask) == 0) {    // NOT NULL
                return static_cast<uint16_t>(i); // 1-based index
            }
        }
        return std::nullopt;
    }

    /// nulls bitmap ==> vector<bool>
    /// - bit == 0 => Not Null
    /// - bit == 1 → Null
    inline std::optional<std::vector<bool>> decode_nulls(tcb::span<const char> buf, uint16_t cc) {
        const size_t need = (static_cast<size_t>(cc) + 7U) / 8U;
        if (buf.size() < need) {
            return std::nullopt;
        }

        const auto* bytes = reinterpret_cast<const uint8_t*>(buf.data());
        std::vector<bool> result;
        result.reserve(cc);

        for (uint16_t i = 0; i < cc; ++i) {
            const uint8_t byte_val = bytes[i / 8];
            const uint8_t bit_mask = static_cast<uint8_t>(1U << (i % 8));

            bool isNull = (byte_val & bit_mask) != 0; // 1이면 Null, 0이면 Not Null
            result.push_back(isNull);
        }

        return result;
    }


    //--------------------------------------------------------------------------------
    /// KDO Common (16 bytes): Change 5.1, 11.x KDO Vector의 공통 헤더
    struct KdoHead {
        uint32_t bdab;   // bdab (4 bytes, offset 0)
        uint32_t hdba;   // hdba (4 bytes, offset 4)
        uint16_t max_fr; // max fr (2 bytes, offset 8)
        uint8_t op;      // operation code (1 byte, offset 10)
        uint8_t xType;   // transaction type (1 byte, offset 11) // flags in OLR
        uint8_t itli;    // itl slot (1 byte, offset 12)
        uint8_t ispac;   //

        bool is_redo() const noexcept { return ora::is_redo(op); }
        bool is_undo() const noexcept { return ora::is_undo(op); }
        bool is_rowDependencies() { return ora::is_rowDependencies(op); }

        /** rType   */ bool is_kdom2() const noexcept { return ora::is_kdom2(xType); }
        /** redo    */ bool is_xa() const noexcept { return ora::is_xa(xType); }
        /** rollback*/ bool is_xr() const noexcept { return ora::is_xr(xType); }
        /** unknown */ bool is_cr() const noexcept { return ora::is_cr(xType); }

        static constexpr auto sz_KdoHead = 16;
        static Result<KdoHead> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO IRP Body - Insert Row Piece - Opcode: 0x02 (Redo), 0x23 (Undo)
    struct KdoIrpBody {
        uint8_t fb;        //  Flag byte
        uint8_t lb;        //  Lock byte
        uint8_t cc;        //  Column count
        uint8_t cki;       //  fb.C
        uint32_t hdba;     //  fb.F && !fb.H | fb.K => pk
        uint16_t hslot;    //  fb.F && !fb.H | fb.K => pk1
        uint32_t nridBdba; //  !fb.L         | fb.K => nk
        uint16_t nridSlot; //  !fb.L         | fb.K => nk1
        uint16_t sizeDelt; //  ----
        uint16_t slot;     //  slot
        uint8_t tabn;      //
        std::vector<bool> nulls;

        std::string fb_string() const { return FB_string(fb); }

        static Result<KdoIrpBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO DRP Body - Delete Row Piece (4 bytes) - Opcode: 0x03 (Redo), 0x22 (Undo)
    struct KdoDrpBody {
        uint16_t slot;
        uint16_t tabn;

        static Result<KdoDrpBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO LKR Body - Lock Row Piece (4 bytes) - Opcode: 0x04 (Redo), 0x24 (Undo)
    struct KdoLkrBody {
        uint16_t slot; //
        uint8_t tabn;  // unknown in AFC
        uint8_t to;    // lock in AFC
        static Result<KdoLkrBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO URP Body - Update Row Piece - Opcode: 0x05 (Redo), 0x25 (Undo)
    struct KdoUrpBody {
        uint8_t fb;          //  Flag byte
        uint8_t lock_byte;   //  Lock byte
        uint8_t ckix;        //  ckix ?
        uint8_t tabn;        //  Table number
        uint16_t slot;       //  Slot
        uint8_t ncol;        //  Total column count in row
        uint8_t nnew;        //  Updated column count in row
        uint16_t size;       //

        std::vector<bool> nulls;
        static Result<KdoUrpBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO ORP Body - Overwrite Row Piece - Opcode: 0x06 (Redo), 0x26 (Undo)
    struct KdoOrpBody {
        uint8_t fb;        // flag byte
        uint8_t lb;        // lock byte
        uint8_t cc;        // column count
        uint8_t cki;       // fb.C
        uint32_t dba2;     // dba2  ---- not in OLR
        uint32_t nridBdba; // !fb.L
        uint32_t nridSlot; // !fb.L
        uint16_t sizeDelt; // sizeDelta
        uint16_t slot;     // slot
        uint8_t tabn;      //

        std::vector<bool> nulls;
        std::string fb_string() const { return FB_string(fb); }
        static Result<KdoOrpBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO MFC Body - Manipulate First Column * - Opcode: 0x07 (Redo), 0x27 (Undo)
    struct KdoMfcBody {
        uint16_t slot;           //  slot
        uint8_t manipulate_code; //  manipulate code --- not sure

        static Result<KdoMfcBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO CFA Body - Change Forwarding Address - Opcode: 0x08 (Redo), 0x28 (Undo)
    struct KdoCfaBody {
        uint32_t nridBdba; //
        uint16_t nridSlot; //
        uint16_t slot;     //
        uint16_t flag;     //
        uint8_t tabn;      //
        uint8_t lock;      //
        static Result<KdoCfaBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO QMI/D Body - Quick Multi Insert/Delete - Opcode: 0x0B, 0x2B, 0x0C, 0x2C
    struct KdoQmBody {
        uint8_t tabn;                //
        uint8_t lock;                //
        uint8_t nrow;                //
        std::vector<uint16_t> slots; // QMD --> slots-Delta
        // see:: Ch_Qmi ::: QMI --> N1 (rowSizes) --> N2 (cols/Delta; dumpCols)

        static Result<KdoQmBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO Lmn Body - Logminer Operations - Opcode: 0x10 (Redo), 0x30 (Undo)
    struct KdoLmnBody { // todo :::
        tcb::span<const char> data;

        static Result<KdoLmnBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO Skl Body - Set Key links - Opcode: 0x0a (Redo), 0x2a (Undo)
    struct KdoSklBody {
        uint32_t fwd;
        uint16_t fwdPos;
        uint32_t bkw;
        uint16_t bkwPos;
        uint8_t fl;
        uint8_t lock;
        uint8_t slot;           // todo :: wrong position in OLR 11 ??

        // fl & 0x01 != 0 ? 'F' : '-'; // fwdFl
        // fl & 0x02 != 0 ? 'B' : '-'; // bkwFl
        static Result<KdoSklBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO Dsc Body - Opcode: 0x0e (Redo), 0x2e (Undo)
    struct KdoDscBody {
        uint16_t slot;
        uint8_t tabn;
        uint8_t rel; // piece relative col. number
        static Result<KdoDscBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /// KDO Cki Body - Change Cluster key Index - Opcode: 0x09(Redo)
    struct KdoCkiBody {
        uint8_t fwd[4];
        uint8_t bkw[4];
        uint16_t fwd_dec;
        uint16_t bkw_dec;
        uint8_t slot;               // todo :: may wrong position in OLR
        uint8_t flag;
        uint8_t lock;

        // fl & 0x01 != 0 ? 'F' : '-'; // fwdFl
        // fl & 0x02 != 0 ? 'B' : '-'; // bkwFl

        static Result<KdoCkiBody> decode(tcb::span<const char> buf, bool isLittle);
    };

    /** KDO fallback */
    struct KdoRawBody {
        tcb::span<const char> data;
    };

    // ----------------------------------------------------------------------------------------------------
    using KdoBody = std::variant<KdoIrpBody, // 0x02, 0x23 (Single Insert) -- ??
                                 KdoDrpBody, // 0x03, 0x22 (Single Delete) -- ??
                                 KdoLkrBody, // 0x04 (Lock Row)
                                 KdoUrpBody, // 0x05 (Single Update)
                                 KdoOrpBody, // 0x06 (Overwrite Row)
                                 KdoMfcBody, // 0x07 (Manipulate First Column)
                                 KdoCfaBody, // 0x08 (Change Forwarding Address)
                                 KdoCkiBody, // 0x09 (Change Cluster key Index)
                                 KdoSklBody, // 0x0A (Set key link)
                                 KdoQmBody,  // 0x0B/0x0C (QMI, QMD)
                                 KdoDscBody, // 0x0E
                                 KdoLmnBody, // 0x10, 0x30 (LogMiner)
                                 KdoRawBody  // fallback
                                 >;

    /// Kdo
    struct KdoVector {
        KdoHead head;
        KdoBody body;

        static Result<KdoVector> decode(tcb::span<const char> buf, bool isLittle);
    };

    // ====================================================================================================
    /// 16 byte
    inline Result<KdoHead> KdoHead::decode(tcb::span<const char> buf, bool isLittle) {
        if (auto sz = sizeof(KdoHead); buf.size() < sz) {
            return err_of(fmt::format("[KdoHead] buf-size ({}) < {}", buf.size(), sz));
        }
        return KdoHead{.bdab   = decode_At<uint32_t>(buf, isLittle, 0),
                       .hdba   = decode_At<uint32_t>(buf, isLittle, 4),
                       .max_fr = decode_At<uint16_t>(buf, isLittle, 8),
                       .op     = decode_At<uint8_t >(buf, isLittle, 10),
                       .xType  = decode_At<uint8_t >(buf, isLittle, 11),
                       .itli   = decode_At<uint8_t >(buf, isLittle, 12),
                       .ispac  = decode_At<uint8_t >(buf, isLittle, 13) };
    }

    inline Result<KdoIrpBody> KdoIrpBody::decode(tcb::span<const char> buf, bool isLittle) {
        constexpr auto sz_IrpBody = 32;

        if ( buf.size() < sz_IrpBody) {
            return err_of(fmt::format("[IrpBody] buf({}) < {}", buf.size(), sz_IrpBody));
        }

        KdoIrpBody out{
            .fb         = decode_At<uint8_t >(buf, isLittle, 0),
            .lb         = decode_At<uint8_t >(buf, isLittle, 1),
            .cc         = decode_At<uint8_t >(buf, isLittle, 2),
            // --------------------------------------------------------------------------------
            .cki        = decode_At<uint8_t >(buf, isLittle, 3),  // fb.C -->  cki
            .hdba       = decode_At<uint32_t>(buf, isLittle, 4),  // fb.Fst && !fb.Hdr --> hrid | fb.K --> pk
            .hslot      = decode_At<uint16_t>(buf, isLittle, 8),  // fb.Fst && !fb.Hdr --> hrid | fb.K --> pk2
            .nridBdba   = decode_At<uint32_t>(buf, isLittle, 12), // !fb.Lst --> nrid           | fb.K --> nk
            .nridSlot   = decode_At<uint16_t>(buf, isLittle, 16), // !fb.Lst --> nrid           | fb.K --> nk1

            // 10~11 :: padding
            // 18 ~ 23 :: unknown(6)          --- todo ::: find curc, comp
            // --------------------------------------------------------------------------------
            .sizeDelt   = decode_At<uint16_t>(buf, isLittle, 24),
            .slot       = decode_At<uint16_t>(buf, isLittle, 26),
            .tabn       = decode_At<uint8_t >(buf, isLittle, 28),
        };

        if (auto nulls = decode_nulls(buf.subspan(29), out.cc)) out.nulls = std::move(*nulls);

        return out;
    }

    inline Result<KdoDrpBody> KdoDrpBody::decode(tcb::span<const char> buf, bool isLittle) {
        constexpr auto sz_DrpBody = 4;
        if ( buf.size() < sz_DrpBody) {
            return err_of(fmt::format("[DrpBody] buf({}) < {}", buf.size(), sz_DrpBody));
        }
        return KdoDrpBody{
            .slot = decode_At<uint16_t>(buf, isLittle, 0),
            .tabn = decode_At<uint16_t>(buf, isLittle, 2)};
    }

    inline Result<KdoLkrBody> KdoLkrBody::decode(tcb::span<const char> buf, bool isLittle) {
        constexpr auto sz_LkrBody = 4;
        if (buf.size() < sz_LkrBody) {
            return err_of(fmt::format("[LkrBody] buf({}) < {}", buf.size(), sz_LkrBody));
        }

        return KdoLkrBody{
                .slot = decode_At<uint16_t>(buf, isLittle, 0), //
                .tabn = decode_At<uint8_t >(buf, isLittle, 2), //
                .to   = decode_At<uint8_t >(buf, isLittle, 3)  //
        };
    }

    inline Result<KdoUrpBody> KdoUrpBody::decode(tcb::span<const char> buf, bool isLittle) {

        constexpr auto sz_UrpBody = 10;
        if ( buf.size() < sz_UrpBody) {
            return err_of(fmt::format("[UrpBody] buf ({}) < {}", buf.size(), sz_UrpBody));
        }

        KdoUrpBody out{.fb          = decode_At<uint8_t >(buf, isLittle, 0),
                       .lock_byte   = decode_At<uint8_t >(buf, isLittle, 1),
                       .ckix        = decode_At<uint8_t >(buf, isLittle, 2),
                       .tabn        = decode_At<uint8_t >(buf, isLittle, 3),
                       .slot        = decode_At<uint16_t>(buf, isLittle, 4),
                       .ncol        = decode_At<uint8_t >(buf, isLittle, 6),
                       .nnew        = decode_At<uint8_t >(buf, isLittle, 7),
                       .size        = decode_At<uint16_t>(buf, isLittle, 8)};

        if (auto nulls = decode_nulls(buf.subspan(10), out.ncol)) out.nulls = std::move(*nulls);

        return out;
    }

    inline Result<KdoOrpBody> KdoOrpBody::decode(tcb::span<const char> buf, bool isLittle) {

        constexpr auto sz_OrpBody = 32;
        if ( buf.size() < sz_OrpBody) {
            return err_of(fmt::format("[OrpBody] buf({}) < {}", buf.size(), sz_OrpBody));
        }

        KdoOrpBody out{
            .fb       = decode_At<uint8_t>(buf, isLittle, 0),       // flag byte
            .lb       = decode_At<uint8_t>(buf, isLittle, 1),       // lock byte
            .cc       = decode_At<uint8_t>(buf, isLittle, 2),       // column count
            .cki      = decode_At<uint8_t>(buf, isLittle, 3),       // fb.C
            .dba2     = decode_At<uint32_t>(buf, isLittle, 4),      // dba2  ---- not in OLR.. TODO :: CHECK
            // 4 ~ 11 :: unknown in OLR.
            .nridBdba = decode_At<uint32_t>(buf, isLittle, 12),     // !fb.L
            .nridSlot = decode_At<uint32_t>(buf, isLittle, 16),     // !fb.L
            // 18 ~ 23 :: unknown
            .sizeDelt = decode_At<uint16_t>(buf, isLittle, 24),     // size/delt
            .slot     = decode_At<uint16_t>(buf, isLittle, 26),     // slot
            .tabn     = decode_At<uint8_t>(buf, isLittle, 28)
        };


        if (auto nulls = decode_nulls(buf.subspan(29), out.cc)) out.nulls = std::move(*nulls);

        return out;
    }

    inline Result<KdoMfcBody> KdoMfcBody::decode(tcb::span<const char> buf, bool isLittle) {
        constexpr auto sz_MfcBody = 4;
        if ( buf.size() < sz_MfcBody) {
            return err_of(fmt::format("[KdoMfcBody] buf({}) < {}", buf.size(), sz_MfcBody));
        }
        KdoMfcBody out{
           .slot = decode_At<uint16_t> (buf, isLittle, 0),
           .manipulate_code = decode_At<uint8_t>(buf, isLittle, 3)};

        return out;
    }

    inline Result<KdoCfaBody> KdoCfaBody::decode(tcb::span<const char> buf, bool isLittle) {
        constexpr auto sz_CfaBody = 16;

        if ( buf.size() < sz_CfaBody) {
            return err_of(fmt::format("[CfaBody] buf({}) < {}", buf.size(), sz_CfaBody));
        }
        KdoCfaBody out{.nridBdba = decode_At<uint32_t>(buf, isLittle, 0),
                       .nridSlot = decode_At<uint16_t>(buf, isLittle, 4),
                       .slot     = decode_At<uint16_t>(buf, isLittle, 8),
                       .flag     = decode_At<uint8_t >(buf, isLittle, 10),
                       .tabn     = decode_At<uint8_t >(buf, isLittle, 11),
                       .lock     = decode_At<uint8_t  >(buf, isLittle, 12)
        };
        return out;
    }

    inline Result<KdoQmBody> KdoQmBody::decode(tcb::span<const char> buf, bool isLittle) {

        if (buf.size() < 8)
            return err_of(fmt::format("[QmBody] buf({}) < 4", buf.size()));

        KdoQmBody out{
            .tabn = decode_At<uint8_t>(buf, isLittle, 0),
            .lock = decode_At<uint8_t>(buf, isLittle, 1),
            .nrow = decode_At<uint8_t>(buf, isLittle, 2),
        };

        auto subspan = buf.subspan(4);

        if (subspan.size() < out.nrow * 2)
            return err_of(fmt::format("[QmBody] buf-slots({}) < {}", subspan.size(), out.nrow * 2));

        out.slots = coral::decode_array<uint16_t>(buf.subspan(4), isLittle, out.nrow);

        return out;
    }

    inline Result<KdoLmnBody> KdoLmnBody::decode(tcb::span<const char> buf, bool isLittle) {
        return KdoLmnBody{buf};
    }

    inline Result<KdoSklBody> KdoSklBody::decode(tcb::span<const char> buf, bool isLittle) {

        if (buf.size() < 14)
            return err_of(fmt::format("[KdoSklBody] buf ({}) < 14", buf.size()));

        return KdoSklBody{.fwd    = decode_At<uint32_t>(buf, isLittle, 0),
                          .fwdPos = decode_At<uint16_t>(buf, isLittle, 4),
                          .bkw    = decode_At<uint32_t>(buf, isLittle, 6),
                          .bkwPos = decode_At<uint16_t>(buf, isLittle, 10),
                          .fl     = decode_At<uint8_t >(buf, isLittle, 12),
                          .lock   = decode_At<uint8_t >(buf, isLittle, 13),
                          .slot   = decode_At<uint8_t >(buf, isLittle, 14)
        };
    }

    inline Result<KdoDscBody> KdoDscBody::decode(tcb::span<const char> buf, bool isLittle) {

        if (buf.size() < 4)
            return err_of(fmt::format("[KdoSklBody] buf ({}) < 4", buf.size()));

        return KdoDscBody{
                .slot = decode_At<uint16_t>(buf, isLittle, 0),
                .tabn = decode_At<uint8_t>(buf, isLittle, 2),
                .rel  = decode_At<uint8_t>(buf, isLittle, 3),
        };
    }

    /// see : https://github.com/bersler/OpenLogReplicator/blob/6bc92bc1b89255fbc491e3080cb12a4c1dd8e832/src/parser/OpCode.h#L1717
    /// - todo :: may wrong decode offset in OLR
    inline Result<KdoCkiBody> KdoCkiBody::decode(tcb::span<const char> buf, bool isLittle) {

        if (buf.size() < 14)
            return err_of(fmt::format("[KdoCkiBody] buf ({}) < 14", buf.size()));

        auto out = KdoCkiBody{
            .slot = decode_At<uint8_t>(buf, isLittle, 14),  // todo :: may wrong position in OLR
            .flag = decode_At<uint8_t>(buf, isLittle, 12),
            .lock = decode_At<uint8_t>(buf, isLittle, 13)
        };

        if ((out.flag & 0x01) != 0) {
            std::memcpy(out.fwd, buf.data(), 4);
            out.fwd_dec = decode_At<uint16_t>(buf, isLittle, 4);
        }

        if ((out.flag & 0x02) != 0) {
            std::memcpy(out.bkw, buf.data() + 6, 4);
            out.bkw_dec = decode_At<uint16_t>(buf, isLittle, 10);
        }

        return out;
    }

    inline Result<KdoVector> KdoVector::decode(tcb::span<const char> buf, bool isLittle) {

        auto head = KdoHead::decode(buf, isLittle);
        if (!head) return tl::make_unexpected(head.error());

        KdoVector out{.head = *head};

        auto rest = buf.subspan(KdoHead::sz_KdoHead);

        using Decoder = std::function<Result<KdoBody>(tcb::span<const char>, bool)>;

        static const std::unordered_map<KdoType, Decoder> decoders = {
            {KdoType::Irp, KdoIrpBody::decode}, {KdoType::Drp, KdoDrpBody::decode},
            {KdoType::Lkr, KdoLkrBody::decode}, {KdoType::Urp, KdoUrpBody::decode},
            {KdoType::Orp, KdoOrpBody::decode}, {KdoType::Mfc, KdoMfcBody::decode},
            {KdoType::Cfa, KdoCfaBody::decode}, {KdoType::Cki, KdoCkiBody::decode},
            {KdoType::Skl, KdoSklBody::decode}, {KdoType::Qmi, KdoQmBody::decode},
            {KdoType::Qmd, KdoQmBody::decode},  {KdoType::Dsc, KdoDscBody::decode},
            {KdoType::Lmn, KdoLmnBody::decode},
    };

        auto kdoType = get_kdoType(head->op);

        auto it = decoders.find(kdoType);
        if (it != decoders.end()) {
            auto body = it->second(rest, isLittle);
            if (!body)
                return tl::make_unexpected(body.error());

            out.body = *body;
        } else {
            out.body = KdoRawBody{rest};
        }

        return out;
    }

    // ----------------------------------------------------------------------------------------------------
    using std::holds_alternative;
    static constexpr bool is_irp(const KdoBody &b) noexcept { return holds_alternative<KdoIrpBody>(b); }
    static constexpr bool is_drp(const KdoBody &b) noexcept { return holds_alternative<KdoDrpBody>(b); }
    static constexpr bool is_lkr(const KdoBody &b) noexcept { return holds_alternative<KdoLkrBody>(b); }
    static constexpr bool is_urp(const KdoBody &b) noexcept { return holds_alternative<KdoUrpBody>(b); }
    static constexpr bool is_orp(const KdoBody &b) noexcept { return holds_alternative<KdoOrpBody>(b); }
    static constexpr bool is_mfc(const KdoBody &b) noexcept { return holds_alternative<KdoMfcBody>(b); }
    static constexpr bool is_cfa(const KdoBody &b) noexcept { return holds_alternative<KdoCfaBody>(b); }
    static constexpr bool is_qm(const KdoBody &b) noexcept { return holds_alternative<KdoQmBody>(b); }
    static constexpr bool is_lwn(const KdoBody &b) noexcept { return holds_alternative<KdoLmnBody>(b); }
    static constexpr bool is_raw(const KdoBody &b) noexcept { return holds_alternative<KdoRawBody>(b); }


    [[nodiscard]] inline bool isCompressed(const KdoBody &body, size_t span_size) noexcept {

        return std::visit(
                [&](const auto &b) -> bool {
                    using T = std::decay_t<decltype(b)>;

                    if constexpr (std::is_same_v<T, KdoIrpBody>) {
                        return (span_size == b.sizeDelt) && (b.cc > 1 || b.cc == 0);
                    }
                    if constexpr (std::is_same_v<T, KdoOrpBody>) {
                        return (span_size == b.sizeDelt) && (b.cc > 1 || b.cc == 0);
                    }

                    return false;
                },
                body);
    }

    [[nodiscard]] inline uint8_t get_cc(const KdoBody &body) noexcept {

        return std::visit(
                [](const auto &b) -> uint8_t {
                    using T = std::decay_t<decltype(b)>;

                    if constexpr (std::is_same_v<T, KdoIrpBody>) { return b.cc; }
                    if constexpr (std::is_same_v<T, KdoOrpBody>) { return b.cc; }

                    assert(false && "expects KdoIrpBody, KdoOrpBody");
                    return 0;
                },
                body);
    }
    [[nodiscard]] inline uint8_t get_nnew(const KdoBody &body) noexcept {

        return std::visit(
                [](const auto &b) -> uint8_t {
                    using T = std::decay_t<decltype(b)>;
                    if constexpr (std::is_same_v<T, KdoUrpBody>) return b.nnew;

                    assert(false && "expects KdoUrpBody");
                    return 0;
                },
                body);
    }
    [[nodiscard]] inline uint8_t get_nrow(const KdoBody &body) noexcept {

        return std::visit(
                [](const auto &b) -> uint8_t {
                    using T = std::decay_t<decltype(b)>;
                    if constexpr (std::is_same_v<T, KdoQmBody>) return b.nrow;

                    assert(false && "expects KdoQmiBody");
                    return 0;
                },
                body);
    }
    // ----------------------------------------------------------------------------------------------------

    [[nodiscard]] static constexpr bool is_mfc(const KdoVector &kdo) noexcept { return is_mfc(kdo.body); }
    [[nodiscard]] static constexpr bool is_irp(const KdoVector &kdo) noexcept { return is_irp(kdo.body); }
    [[nodiscard]] static constexpr bool is_drp(const KdoVector &kdo) noexcept { return is_drp(kdo.body); }
    [[nodiscard]] static constexpr bool is_urp(const KdoVector &kdo) noexcept { return is_urp(kdo.body); }
    [[nodiscard]] static constexpr bool is_orp(const KdoVector &kdo) noexcept { return is_orp(kdo.body); }
    [[nodiscard]] static constexpr bool is_lkr(const KdoVector &kdo) noexcept { return is_lkr(kdo.body); }

    // ----------------------------------------------------------------------------------------------------

    inline std::string to_string(const KdoHead &h) {
        return fmt::format(
            "Hdr {{bdab: 0x{:08x}, hdba: 0x{:08x}, max_fr: {}, op: 0x{:02x}, xType: 0x{:02x}, itli: {}, ispac: {}}}",
            h.bdab, h.hdba, h.max_fr, h.op, h.xType, h.itli, h.ispac
        );
    }

    inline std::string to_string(const KdoIrpBody &b) {
        return fmt::format(
            "Irp {{fb: 0x{:02x} ({}), lb: {}, cc: {}, cki: {}, hdba: 0x{:08x}, hslot: {}, nridBdba: 0x{:08x}, nridSlot: {}, size: {}, slot: {}, tabn: {}}}",
            b.fb, b.fb_string(), b.lb, b.cc, b.cki, b.hdba, b.hslot, b.nridBdba, b.nridSlot, b.sizeDelt, b.slot, b.tabn
        );
    }

    inline std::string to_string(const KdoDrpBody &b) {
        return fmt::format("Drp {{slot: {}, tabn: {}}}", b.slot, b.tabn);
    }

    inline std::string to_string(const KdoLkrBody &b) {
        return fmt::format("Lkr {{slot: {}, tabn: {}, to: {}}}", b.slot, b.tabn, b.to);
    }

    inline std::string to_string(const KdoUrpBody &b) {
        return fmt::format(
            "Urp {{fb: 0x{:02x}, lb: {}, ckix: {}, tabn: {}, slot: {}, ncol: {}, nnew: {}, size: {}}}",
            b.fb, b.lock_byte, b.ckix, b.tabn, b.slot, b.ncol, b.nnew, b.size
        );
    }

    inline std::string to_string(const KdoOrpBody &b) {
        return fmt::format(
            "Orp {{fb: 0x{:02x} ({}), lb: {}, cc: {}, cki: {}, dba2: 0x{:08x}, nridBdba: 0x{:08x}, nridSlot: {}, size: {}, slot: {}, tabn: {}}}",
            b.fb, b.fb_string(), b.lb, b.cc, b.cki, b.dba2, b.nridBdba, b.nridSlot, b.sizeDelt, b.slot, b.tabn
        );
    }

    inline std::string to_string(const KdoMfcBody &b) {
        return fmt::format("Mfc {{slot: {}, manipulate_code: 0x{:02x}}}", b.slot, b.manipulate_code);
    }

    inline std::string to_string(const KdoCfaBody &b) {
        return fmt::format(
            "Cfa {{nridBdba: 0x{:08x}, nridSlot: {}, slot: {}, flag: 0x{:04x}, tabn: {}, lock: {}}}",
            b.nridBdba, b.nridSlot, b.slot, b.flag, b.tabn, b.lock
        );
    }

    inline std::string to_string(const KdoCkiBody &b) {
        return fmt::format(
            "Cki {{fwd_dec: {}, bkw_dec: {}, slot: {}, flag: 0x{:02x}, lock: {}}}",
            b.fwd_dec, b.bkw_dec, b.slot, b.flag, b.lock
        );
    }

    inline std::string to_string(const KdoSklBody &b) {
        return fmt::format(
            "Skl {{fwd: 0x{:08x}, fwdPos: {}, bkw: 0x{:08x}, bkwPos: {}, fl: 0x{:02x}, lock: {}, slot: {}}}",
            b.fwd, b.fwdPos, b.bkw, b.bkwPos, b.fl, b.lock, b.slot
        );
    }

    inline std::string to_string(const KdoQmBody &b) {
        return fmt::format(
            "Qm {{tabn: {}, lock: {}, nrow: {}, slots_count: {}}}",
            b.tabn, b.lock, b.nrow, b.slots.size()
        );
    }

    inline std::string to_string(const KdoDscBody &b) {
        return fmt::format("Dsc {{slot: {}, tabn: {}, rel: {}}}", b.slot, b.tabn, b.rel);
    }

    inline std::string to_string(const KdoLmnBody &b) {
        return fmt::format("Lmn {{data_size: {}}}", b.data.size());
    }

    inline std::string to_string(const KdoRawBody &b) {
        return fmt::format("Raw {{data_size: {}}}", b.data.size());
    }

    inline std::string to_string(const KdoBody &body) {
        return std::visit([](const auto &b) { return to_string(b); }, body);
    }

    inline std::string to_string(const KdoVector &v) {
        return fmt::format("  KDO {{\n    {},\n    {}\n  }}", to_string(v.head), to_string(v.body));
    }
}
