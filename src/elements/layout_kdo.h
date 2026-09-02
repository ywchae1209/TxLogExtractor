#pragma once

#include <cstdint>
#include "tcb/span.hpp"
#include "../coral_decode.h"
#include <variant>

//// https://lab.idatabank.com/confluence/pages/viewpage.action?pageId=119020766#Redologstructure-KDOvector(KernalDataOperation)
namespace ora {

    using coral::decode_at;
    using std::optional;

    enum class KdoType : uint8_t {
        Irp,    // Single Insert (0x02, 0x23)
        Drp,    // Single Delete (0x03, 0x22)
        Lkr,    // Lock Row (0x04, 0x24)
        Urp,    // Single Update (0x05, 0x25)
        Orp,    // Overwrite Row (0x06, 0x26)
        Mfc,    // Manipulate First Column (0x07, 0x27)
        Cfa,    // Change Forwarding Address (0x08, 0x28)
        Qmi,    // Quick Multi-Insert (0x0B, 0x2B)
        Qmd,    // Quick Multi-Delete (0x0C, 0x2C)
        Lmn,    // Logminer (0x10, 0x30)
        Unknown
    };

#pragma pack(push, 1)
    //// Kdo common head
    struct KdoHead {
        uint32_t bdab;        // bdab (4 bytes, offset 0)
        uint32_t hdba;        // hdba (4 bytes, offset 4)
        uint16_t max_fr;      // max fr (2 bytes, offset 8)
        uint8_t  op_code;     // operation code (1 byte, offset 10)
        uint8_t  tx_type;     // transaction type (1 byte, offset 11)
        uint8_t  itl_slot;    // itl slot (1 byte, offset 12)
        uint8_t  unknown[3];  // unknown[3] (3 bytes, offset 13~15)

        [[nodiscard]] constexpr bool is_redo() const noexcept { return op_code < 0x20; }
        [[nodiscard]] constexpr bool is_undo() const noexcept { return op_code >= 0x20; }

        [[nodiscard]] constexpr KdoType get_type() const noexcept {
            // Redo/Undo의 차이(0x20 오프셋)를 제거
            const uint8_t base_op = is_undo() ? (op_code - 0x20) : op_code;

            switch (base_op) {
                case 0x02: return KdoType::Irp;
                case 0x03: return KdoType::Drp;
                case 0x04: return KdoType::Lkr;
                case 0x05: return KdoType::Urp;
                case 0x06: return KdoType::Orp;
                case 0x07: return KdoType::Mfc;
                case 0x08: return KdoType::Cfa;
                case 0x0B: return KdoType::Qmi;
                case 0x0C: return KdoType::Qmd;
                case 0x10: return KdoType::Lmn;
                default:   return KdoType::Unknown;
            }
        }
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoHead decode_kdo_head0(tcb::span<const char> buf) {

        KdoHead res;

        res.bdab       = decode_at<uint32_t, IsLittle>(buf, 0);
        res.hdba       = decode_at<uint32_t, IsLittle>(buf, 4);
        res.max_fr     = decode_at<uint16_t, IsLittle>(buf, 8);
        res.op_code    = decode_at<uint8_t,  IsLittle>(buf, 10);
        res.tx_type    = decode_at<uint8_t,  IsLittle>(buf, 11);
        res.itl_slot   = decode_at<uint8_t,  IsLittle>(buf, 12);
        res.unknown[0] = decode_at<uint8_t,  IsLittle>(buf, 13);
        res.unknown[1] = decode_at<uint8_t,  IsLittle>(buf, 14);
        res.unknown[2] = decode_at<uint8_t,  IsLittle>(buf, 15);

        return res;
    }

    [[nodiscard]] inline optional<KdoHead> decode_kdo_head(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoHead)) { // sizeof(KdoHead) == 16
            return std::nullopt;
        }

        return isLittle ? decode_kdo_head0<true>(buf)
                        : decode_kdo_head0<false>(buf);
    }

#pragma pack(push, 1)
    //// op 2 --------------------------------------------------------------------------------\n
    //// Insert :: KdoIrp == KdoHead + KdoIrpBody
    struct KdoIrpBody {
        uint8_t  flag_byte;   // flag byte (1 byte, offset 0)
        uint8_t  lock_byte;   // lock byte (1 byte, offset 1)
        uint8_t  cc;          // column count (1 byte, offset 2)
        uint8_t  unknown0;    // unknown (1 byte, offset 3)

        uint32_t hdba;        // head DBA (4 bytes, offset 4)

        uint16_t unknown1;    // unknown (2 bytes, offset 8)
        uint16_t hslot;       // head Slot (2 bytes, offset 10)     todo :: --encoding

        uint32_t ndba;        // next row DBA (4 bytes, offset 12)

        uint16_t unknown2;    // unknown (2 bytes, offset 16)
        uint16_t nslot;       // next row slot (2 bytes, offset 18) todo :: --encoding

        uint32_t unknown3;    // unknown (4 bytes, offset 20)

        uint16_t size;        // size (2 bytes, offset 24)
        uint16_t slot;        // slot (2 bytes, offset 26)

        uint8_t  unknown4;    // unknown (1 byte, offset 28)        todo :: isCmdRow ???
        uint8_t  unknown5;    // unknown (1 byte, offset 29)
        uint16_t unknown6;    // unknown (2 bytes, offset 30)

        uint32_t unknown7;    // unknown (4 bytes, offset 32)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoIrpBody decode_kdo_irp_body0(tcb::span<const char> buf) {
        KdoIrpBody res;

        res.flag_byte = decode_at<uint8_t,  IsLittle>(buf, 0);
        res.lock_byte = decode_at<uint8_t,  IsLittle>(buf, 1);
        res.cc        = decode_at<uint8_t,  IsLittle>(buf, 2);
        res.unknown0  = decode_at<uint8_t,  IsLittle>(buf, 3);
        res.hdba      = decode_at<uint32_t, IsLittle>(buf, 4);
        res.unknown1  = decode_at<uint16_t, IsLittle>(buf, 8);
        res.hslot     = decode_at<uint16_t, IsLittle>(buf, 10);
        res.ndba      = decode_at<uint32_t, IsLittle>(buf, 12);
        res.unknown2  = decode_at<uint16_t, IsLittle>(buf, 16);
        res.nslot     = decode_at<uint16_t, IsLittle>(buf, 18);
        res.unknown3  = decode_at<uint32_t, IsLittle>(buf, 20);
        res.size      = decode_at<uint16_t, IsLittle>(buf, 24);
        res.slot      = decode_at<uint16_t, IsLittle>(buf, 26);
        res.unknown4  = decode_at<uint8_t,  IsLittle>(buf, 28);
        res.unknown5  = decode_at<uint8_t,  IsLittle>(buf, 29);
        res.unknown6  = decode_at<uint16_t, IsLittle>(buf, 30);
        res.unknown7  = decode_at<uint32_t, IsLittle>(buf, 32);

        return res;
    }

    [[nodiscard]] inline optional<KdoIrpBody> decode_kdo_irp_body(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoIrpBody)) { // sizeof(KdoIrpBody) == 36
            return std::nullopt;
        }

        return isLittle ? decode_kdo_irp_body0<true>(buf)
                        : decode_kdo_irp_body0<false>(buf);
    }

#pragma pack(push, 1)
    //// op 3 --------------------------------------------------------------------------------\n
    //// Delete :: KdoDrp == KdoHead + KdoDrpBody
    struct KdoDrpBody {
        uint16_t size;  // size (2 bytes, offset 0)
        uint16_t slot;  // slot (2 bytes, offset 2)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoDrpBody decode_kdo_drp_body0(tcb::span<const char> buf) {
        KdoDrpBody res;

        res.size = decode_at<uint16_t, IsLittle>(buf, 0);
        res.slot = decode_at<uint16_t, IsLittle>(buf, 2);

        return res;
    }

    [[nodiscard]] inline optional<KdoDrpBody> decode_kdo_drp_body(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoDrpBody)) { // sizeof(KdoDrpBody) == 4
            return std::nullopt;
        }

        return isLittle ? decode_kdo_drp_body0<true>(buf)
                        : decode_kdo_drp_body0<false>(buf);
    }


#pragma pack(push, 1)
    //// op 4 --------------------------------------------------------------------------------\n
    //// Lock :: KdoLkr == KdoHead + KdoLkrBody
    struct KdoLkrBody {
        uint16_t slot;     // slot (2 bytes, offset 0)
        uint8_t  unknown;  // unknown (1 byte, offset 2)
        uint8_t  lock;     // lock (1 byte, offset 3)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoLkrBody decode_kdo_lkr_body0(tcb::span<const char> buf) {
        KdoLkrBody res;

        res.slot    = decode_at<uint16_t, IsLittle>(buf, 0);
        res.unknown = decode_at<uint8_t,  IsLittle>(buf, 2);
        res.lock    = decode_at<uint8_t,  IsLittle>(buf, 3);

        return res;
    }

    [[nodiscard]] inline optional<KdoLkrBody> decode_kdo_lkr_body(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoLkrBody)) { // sizeof(KdoLkrBody) == 4
            return std::nullopt;
        }

        return isLittle ? decode_kdo_lkr_body0<true>(buf)
                        : decode_kdo_lkr_body0<false>(buf);
    }

#pragma pack(push, 1)
    //// op 5 --------------------------------------------------------------------------------\n
    //// Update :: KdoUrp == KdoHead + KdoUrpBody
    struct KdoUrpBody {
        uint8_t  flag_byte;   // flag byte (1 byte, offset 0)
        uint8_t  lock_byte;   // lock byte (1 byte, offset 1)
        uint8_t  ckix;        // ckix (1 byte, offset 2)
        uint8_t  tabn;        // tabn (1 byte, offset 3)

        uint16_t slot;        // slot (2 bytes, offset 4)
        uint8_t  ncol;        // total column count in row (1 byte, offset 6)
        uint8_t  nnew;        // updated column count in row (1 byte, offset 7)

        uint16_t size;        // size (2 bytes, offset 8)
        uint8_t  unknown0[3]; // unknown[3] (3 bytes, offset 10~12)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoUrpBody decode_kdo_urp_body0(tcb::span<const char> buf) {
        KdoUrpBody res;

        res.flag_byte   = decode_at<uint8_t,  IsLittle>(buf, 0);
        res.lock_byte   = decode_at<uint8_t,  IsLittle>(buf, 1);
        res.ckix        = decode_at<uint8_t,  IsLittle>(buf, 2);
        res.tabn        = decode_at<uint8_t,  IsLittle>(buf, 3);
        res.slot        = decode_at<uint16_t, IsLittle>(buf, 4);
        res.ncol        = decode_at<uint8_t,  IsLittle>(buf, 6);
        res.nnew        = decode_at<uint8_t,  IsLittle>(buf, 7);
        res.size        = decode_at<uint16_t, IsLittle>(buf, 8);
        res.unknown0[0] = decode_at<uint8_t,  IsLittle>(buf, 10);
        res.unknown0[1] = decode_at<uint8_t,  IsLittle>(buf, 11);
        res.unknown0[2] = decode_at<uint8_t,  IsLittle>(buf, 12);

        return res;
    }

    [[nodiscard]] inline optional<KdoUrpBody> decode_kdo_urp_body(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoUrpBody)) { // sizeof(KdoUrpBody) == 13
            return std::nullopt;
        }

        return isLittle ? decode_kdo_urp_body0<true>(buf)
                        : decode_kdo_urp_body0<false>(buf);
    }

#pragma pack(push, 1)
    //// op 6 --------------------------------------------------------------------------------\n
    //// Overwrite :: KdoOrp == KdoHead + KdoOrpBody
    //// sizeof(KdoOrpBody) == 29
    struct KdoOrpBody {
        uint8_t  flag_byte;  // flag byte (1 byte, offset 0)
        uint8_t  lock_byte;  // lock byte (1 byte, offset 1)
        uint8_t  cc;         // column count (1 byte, offset 2)
        uint8_t  unknown0;   // unknown (1 byte, offset 3)

        uint32_t dba2;       // dba2 (4 bytes, offset 4)

        uint16_t unknown1;   // unknown (2 bytes, offset 8)
        uint16_t unknown2;   // unknown (2 bytes, offset 10)

        uint32_t unknown3;   // unknown (4 bytes, offset 12)
        uint32_t unknown4;   // unknown (4 bytes, offset 16)
        uint32_t unknown5;   // unknown (4 bytes, offset 20)

        uint16_t size;       // size (2 bytes, offset 24)
        uint16_t slot;       // slot (2 bytes, offset 26)
        uint8_t  tabn;       // table number (1 byte, offset 28)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoOrpBody decode_kdo_orp_body0(tcb::span<const char> buf) {
        KdoOrpBody res;

        res.flag_byte = decode_at<uint8_t,  IsLittle>(buf, 0);
        res.lock_byte = decode_at<uint8_t,  IsLittle>(buf, 1);
        res.cc        = decode_at<uint8_t,  IsLittle>(buf, 2);
        res.unknown0  = decode_at<uint8_t,  IsLittle>(buf, 3);
        res.dba2      = decode_at<uint32_t, IsLittle>(buf, 4);
        res.unknown1  = decode_at<uint16_t, IsLittle>(buf, 8);
        res.unknown2  = decode_at<uint16_t, IsLittle>(buf, 10);
        res.unknown3  = decode_at<uint32_t, IsLittle>(buf, 12);
        res.unknown4  = decode_at<uint32_t, IsLittle>(buf, 16);
        res.unknown5  = decode_at<uint32_t, IsLittle>(buf, 20);
        res.size      = decode_at<uint16_t, IsLittle>(buf, 24);
        res.slot      = decode_at<uint16_t, IsLittle>(buf, 26);
        res.tabn      = decode_at<uint8_t,  IsLittle>(buf, 28);

        return res;
    }

    [[nodiscard]] inline std::optional<KdoOrpBody> decode_kdo_orp_body(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoOrpBody)) { // sizeof(KdoOrpBody) == 29
            return std::nullopt;
        }

        return isLittle ? decode_kdo_orp_body0<true>(buf)
                        : decode_kdo_orp_body0<false>(buf);
    }

#pragma pack(push, 1)
    //// op 7 --------------------------------------------------------------------------------\n
    //// Manipulate first col :: KdoMfc == KdoHead + KdoMfcBody
    struct KdoMfcBody {
        uint16_t slot;            // slot (2 bytes, offset 0)
        uint8_t  unknown0;        // unknown (1 byte, offset 2)
        uint8_t  manipulate_code; // manipulate code (1 byte, offset 3) -- not certain.
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoMfcBody decode_kdo_mfc_body0(tcb::span<const char> buf) {
        KdoMfcBody res;

        res.slot            = decode_at<uint16_t, IsLittle>(buf, 0);
        res.unknown0        = decode_at<uint8_t,  IsLittle>(buf, 2);
        res.manipulate_code = decode_at<uint8_t,  IsLittle>(buf, 3);

        return res;
    }

    [[nodiscard]] inline std::optional<KdoMfcBody> decode_kdo_mfc_body(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoMfcBody)) { // sizeof(KdoMfcBody) == 4
            return std::nullopt;
        }

        return isLittle ? decode_kdo_mfc_body0<true>(buf)
                        : decode_kdo_mfc_body0<false>(buf);
    }

#pragma pack(push, 1)
    //// op 8 --------------------------------------------------------------------------------\n
    //// Change Forward Address :: KdoCfa == KdoHead + KdoCfaBody
    struct KdoCfaBody {
        uint32_t change_dba;  // change dba (4 bytes, offset 0)

        uint16_t change_slot; // change slot (2 bytes, offset 4) todo check ++endian
        uint16_t unknown0;    // unknown (2 bytes, offset 6)

        uint16_t slot;        // slot (2 bytes, offset 8)
        uint16_t unknown1;    // unknown (2 bytes, offset 10)

        uint32_t unknown2;    // unknown (4 bytes, offset 12)
    };
#pragma pack(pop)

    template <bool IsLittle>
    inline KdoCfaBody decode_kdo_cfa_body0(tcb::span<const char> buf) {
        KdoCfaBody res;

        res.change_dba  = decode_at<uint32_t, IsLittle>(buf, 0);
        res.change_slot = decode_at<uint16_t, IsLittle>(buf, 4);
        res.unknown0    = decode_at<uint16_t, IsLittle>(buf, 6);
        res.slot        = decode_at<uint16_t, IsLittle>(buf, 8);
        res.unknown1    = decode_at<uint16_t, IsLittle>(buf, 10);
        res.unknown2    = decode_at<uint32_t, IsLittle>(buf, 12);

        return res;
    }

    [[nodiscard]] inline optional<KdoCfaBody> decode_kdo_cfa_body(tcb::span<const char> buf, bool isLittle) {
        if (buf.size() < sizeof(KdoCfaBody)) { // sizeof(KdoCfaBody) == 16
            return std::nullopt;
        }

        return isLittle ? decode_kdo_cfa_body0<true>(buf)
                        : decode_kdo_cfa_body0<false>(buf);
    }

    //// op 11 --------------------------------------------------------------------------------\n
    //// Quick Multi Insert/Delete :: KdoQmi == KdoHead + KdoQmiBody
    struct KdoQmiBody {
        uint16_t unknown;            // unknown (2 bytes, offset 0)
        uint16_t nrow;               // number of rows (2 bytes, offset 2)  todo:: endian check
        std::vector<uint16_t> slots; // slot[nrow] (2 bytes * nrow, offset 4부터 시작)
    };

    template <bool IsLittle>
    inline KdoQmiBody decode_kdo_qmi_body0(tcb::span<const char> buf, uint16_t nrow) {
        KdoQmiBody res;

        res.unknown = decode_at<uint16_t, IsLittle>(buf, 0);
        res.nrow    = nrow;

        res.slots.reserve(nrow);
        size_t slot_offset = 4;

        for (uint16_t i = 0; i < nrow; ++i) {
            res.slots.push_back(decode_at<uint16_t, IsLittle>(buf, slot_offset));
            slot_offset += sizeof(uint16_t);
        }

        return res;
    }

    [[nodiscard]] inline optional<KdoQmiBody> decode_kdo_qmi_body(tcb::span<const char> buf, bool isLittle) {

        if (buf.size() < 4) return std::nullopt;
        const uint16_t nRow = isLittle
                                  ? decode_at<uint16_t, true>(buf, 2)
                                  : decode_at<uint16_t, false>(buf, 2);

        const auto need = 4 + sizeof(uint16_t) * nRow;
        if (buf.size() < need) return std::nullopt;

        return isLittle ? decode_kdo_qmi_body0<true>(buf, nRow)
                        : decode_kdo_qmi_body0<false>(buf, nRow);
    }

    // ----------------------------------------------------------------------------------------------------
    struct KdoRawBody {
        tcb::span<const char> data;
    };

    // ----------------------------------------------------------------------------------------------------
    using KdoBody = std::variant<
        KdoIrpBody,         // 0x02, 0x23 (Single Insert)
        KdoDrpBody,         // 0x03, 0x22 (Single Delete)
        KdoLkrBody,         // 0x04, 0x24 (Lock Row)
        KdoUrpBody,         // 0x05, 0x25 (Single Update)
        KdoOrpBody,         // 0x06, 0x26 (Overwrite Row)
        KdoMfcBody,         // 0x07, 0x27 (Manipulate First Column)
        KdoCfaBody,         // 0x08, 0x28 (Change Forwarding Address)
        KdoQmiBody,         // 0x0B, 0x2B, 0x0C, 0x2C (QMI, QMD)
        KdoRawBody          // 기타 (LMN..)
    >;

    // ----------------------------------------------------------------------------------------------------
    struct KdoVector {
        KdoHead head;
        KdoBody body;
    };

    [[nodiscard]] inline optional<KdoVector> decode_kdo(tcb::span<const char> buf, bool isLittle) {

        auto head_opt = decode_kdo_head(buf, isLittle);
        if (!head_opt) return std::nullopt;

        KdoVector result;
        result.head = *head_opt;

        auto body_buf = buf.subspan(sizeof(KdoHead));

        switch (result.head.get_type()) {
            case KdoType::Irp: // Single Insert (Redo: 0x02, Undo: 0x23)
            {
                auto body = decode_kdo_irp_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }
            case KdoType::Drp: // Single Delete (Redo: 0x03, Undo: 0x22)
            {
                auto body = decode_kdo_drp_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }
            case KdoType::Lkr: // Lock Row (Redo: 0x04, Undo: 0x24)
            {
                auto body = decode_kdo_lkr_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }
            case KdoType::Urp: // Single Update (Redo: 0x05, Undo: 0x25)
            {
                auto body = decode_kdo_urp_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }
            case KdoType::Orp: // Overwrite Row (Redo: 0x06, Undo: 0x26)
            {
                auto body = decode_kdo_orp_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }
            case KdoType::Mfc: // Manipulate First Column (Redo: 0x07, Undo: 0x27)
            {
                auto body = decode_kdo_mfc_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }
            case KdoType::Cfa: // Change Forwarding Address (Redo: 0x08, Undo: 0x28)
            {
                auto body = decode_kdo_cfa_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }
            case KdoType::Qmi: // Quick Multi-Insert (Redo: 0x0B, Undo: 0x2B)
            case KdoType::Qmd: // Quick Multi-Delete (Redo: 0x0C, Undo: 0x2C)
            {
                auto body = decode_kdo_qmi_body(body_buf, isLittle);
                if (!body) return std::nullopt;
                result.body = *body;
                break;
            }

            // fallback (LMN, Unknown...)
            default:
            {
                result.body = KdoRawBody{ body_buf };
                break;
            }
        }
        return result;
    }

    /*
    void process_kdo(const KdoVector& kdo) {
        // 1. IRP (Single Insert) 타입인지 확인
        if (std::holds_alternative<KdoIrpBody>(kdo.body)) {
            const auto& irp = std::get<KdoIrpBody>(kdo.body);
            // irp.hdba, irp.slot 등 자유롭게 사용
            std::cout << "Insert Row Piece - Slot: " << irp.slot << "\n";
        }
        // 2. DRP (Single Delete) 타입인지 확인
        else if (std::holds_alternative<KdoDrpBody>(kdo.body)) {
            const auto& drp = std::get<KdoDrpBody>(kdo.body);
            std::cout << "Delete Row Piece - Slot: " << drp.slot << "\n";
        }
        // 3. QMI (Multi Insert) 타입인지 확인
        else if (std::holds_alternative<KdoQmiBody>(kdo.body)) {
            const auto& qmi = std::get<KdoQmiBody>(kdo.body);
            std::cout << "Quick Multi-Insert - Rows: " << qmi.nrow << "\n";
            for (uint16_t slot : qmi.slots) {
                // slots 순회
            }
        }
        // 4. 처리되지 않은 Raw 버퍼인 경우
        else if (std::holds_alternative<KdoRawBody>(kdo.body)) {
            const auto& raw = std::get<KdoRawBody>(kdo.body);
            std::cout << "Raw Body Size: " << raw.data.size() << "\n";
        }
    }

    // std::visit + overloaded
    template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

    void process_kdo_with_visit(const KdoVector& kdo) {
        std::visit(overloaded {
            [](const KdoIrpBody& irp) {
                std::cout << "[IRP] Inserted at slot: " << irp.slot << "\n";
            },
            [](const KdoDrpBody& drp) {
                std::cout << "[DRP] Deleted slot: " << drp.slot << "\n";
            },
            [](const KdoQmiBody& qmi) {
                std::cout << "[QMI] Multi-Insert row count: " << qmi.nrow << "\n";
            },
            [](const KdoUrpBody& urp) {
                std::cout << "[URP] Updated slot: " << urp.slot << "\n";
            },
            [](const auto& rest) { // 나머지 타입들(LKR, CFA, Raw)을 한꺼번에 폴백 처리
                std::cout << "[Other Payload]\n";
            }
        }, kdo.body);
    }
     */
}

