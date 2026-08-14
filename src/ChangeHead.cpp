
#include <optional>

#include "ora_layout.h"
#include "ChangeHead.h"

#include "oara_opCodes.h"

/// https://renenyffenegger.ch/notes/index.html

namespace ora {

#pragma pack(push, 1)

    ///< cls
    ///<    1: Data Block (일반 테이블/인덱스 데이터 블록)
    ///<    2: Sort Block
    ///<    3: Undo Header (트랜잭션/롤백 세그먼트 헤더)
    ///<    4: Undo Block (실제 Undo 데이터 블록)
    ///<    5: Space Header (Segment Header / Free List / Bitmap 헤더)
    ///<    7~10: System Undo 관련 블록
    ///<
    ///< dba
    ///<    상위 10bit ((dba >> 22) & 0x3FF): Relative File Number (RFILE#)
    ///<    하위 22bit (dba & 0x3FFFFF): 해당 파일 내의 Block Number (BLOCK#)
    ///<
    ///< ctype
    ///<     0 (Normal): 일반적인 로깅 작업
    ///<     1 (Direct Load): Direct Path Loading (INSERT /*+ APPEND */ 등) 작업의 Redo Record
    ///<     2 (Cleanout): Block Cleanout Record (지연된 커밋 청소 작업)
    ///<    128 : encrypted 여부 지정
    ///<
    ///< con_id
    ///<     0 또는 1: CDB$ROOT
    ///<     2: PDB$SEED
    ///<     3 이상: 각 PDB(Pluggable Database)의 고유 ID
    ///<
    ///< obj_id == Data-Object-Id
    ///<         Object_id: 데이터 딕셔너리 상의 논리적 객체 번호 (테이블 생성 시 부여되며 거의 변경되지 않음)
    ///<    Data_Object_id: 실제 데이터가 저장되는 물리적 세그먼트 번호

    struct ChangeHead_lo {
        uint8_t opLayer; /// [5: Undo/Tx, 10: Index, 11: DataBlock...]
        uint8_t opCode;

        uint16_t cls;           /// * Block-Class ID ~ may contain usn(undo segment number)
        uint32_t afn_obj;       /// * AFN (low 2) + boj_high( High 2)
        uint32_t dba;           /// * Data Block Address

        SCN_lo scn;             /// ignore wrap-high

        uint8_t seq;            /// Change Sequence Number : 동일한 Redo Record 또는 동일 SCN 내에서 해당 체인지 벡터의 순번
        uint8_t ctype;          /// * Change Type
        uint16_t obj_low;       /// * Data-Object-Id-Low

    };
    struct ChangeHead_ext_lo {

        uint8_t con_id;
        uint8_t flag[3];
        uint32_t aux;
    };

#pragma pack(pop)

    std::string to_string(BlockClassType cls) {
        switch (cls) {
            case BlockClassType::DataBlock:             return {"Data_block"};
            case BlockClassType::SortBlock:             return {"Sort_block"};
            case BlockClassType::SaveUndoBlock:         return {"Save_undo_block"};
            case BlockClassType::SegmentHeader:         return {"Segment_header"};
            case BlockClassType::SaveUndoHeader:        return {"Save_undo_header"};
            case BlockClassType::FreeListBlock:         return {"Free_list_block"};
            case BlockClassType::ExtentMapBlock:        return {"Extent_map_block"};
            case BlockClassType::Bmb1st:                return {"1st_level_bmb"};
            case BlockClassType::Bmb2nd:                return {"2nd_level_bmb"};
            case BlockClassType::Bmb3rd:                return {"3rd_level_bmb"};
            case BlockClassType::BitmapBlock:           return {"Bitmap_block"};
            case BlockClassType::BitmapIndexBlock:      return {"Bitmap_index_block"};
            case BlockClassType::FileHeaderBlock:       return {"File_header_block"};
            case BlockClassType::DeferredRollbackBlock: return {"Deferred_rollback_block"};
            case BlockClassType::SystemUndoHeader:      return {"SYSTEM_Undo_header"};
            case BlockClassType::SystemUndoBlock:       return {"SYSTEM_Undo_block"};
            case BlockClassType::UndoHeader:            return {"Undo_Header"};
            case BlockClassType::UndoBlock:             return {"Undo_Block"};
            default:                                    return {"Unknown"};
        }
    }

    static auto decode_cls(uint16_t c0, bool isLittle) noexcept {

        auto c = coral::decode(c0, isLittle);

        /*
            읽는 법 연습하기
            0x0F / 0x10	15 / 16	SYSTEM Undo Header / Block	USN 0
            0x11 / 0x12	17 / 18	Undo Header / Block	USN 1
            0x13 / 0x14	19 / 20	Undo Header / Block	USN 2
            0x15 / 0x16	21 / 22	Undo Header / Block	USN 3
            0x17 / 0x18	23 / 24	Undo Header / Block	USN 4
            0x19 / 0x1A	25 / 26	Undo Header / Block	USN 5
            0x1B / 0x1C	27 / 28	Undo Header / Block	USN 6
            0x1D / 0x1E	29 / 30	Undo Header / Block	USN 7
            0x1F / 0x20	31 / 32	Undo Header / Block	USN 8
         */

        const uint16_t usn = (c < 17) ? 0 : (c - 15) / 2; // Undo Segment Number (Undo 관련 블록일 때)
        const uint16_t cls = (c < 17) ? c : 18 - (c & 1);

        switch (cls) {
            case 1:  return std::make_tuple(BlockClassType::DataBlock, usn);
            case 2:  return std::make_tuple(BlockClassType::SortBlock, usn);
            case 3:  return std::make_tuple(BlockClassType::SaveUndoBlock, usn);
            case 4:  return std::make_tuple(BlockClassType::SegmentHeader, usn);
            case 5:  return std::make_tuple(BlockClassType::SaveUndoHeader, usn);
            case 6:  return std::make_tuple(BlockClassType::FreeListBlock, usn);
            case 7:  return std::make_tuple(BlockClassType::ExtentMapBlock, usn);
            case 8:  return std::make_tuple(BlockClassType::Bmb1st, usn);
            case 9:  return std::make_tuple(BlockClassType::Bmb2nd, usn);
            case 10: return std::make_tuple(BlockClassType::Bmb3rd, usn);
            case 11: return std::make_tuple(BlockClassType::BitmapBlock, usn);
            case 12: return std::make_tuple(BlockClassType::BitmapIndexBlock, usn);
            case 13: return std::make_tuple(BlockClassType::FileHeaderBlock, usn);
            case 14: return std::make_tuple(BlockClassType::DeferredRollbackBlock, usn);
            case 15: return std::make_tuple(BlockClassType::SystemUndoHeader, usn);
            case 16: return std::make_tuple(BlockClassType::SystemUndoBlock, usn);
            case 17: return std::make_tuple(BlockClassType::UndoHeader, usn);
            case 18: return std::make_tuple(BlockClassType::UndoBlock, usn);
            default: return std::make_tuple(BlockClassType::Unknown, usn);
        }
    }

    std::string to_string(ChangeHead& h) {

        const auto desc = opCode_string(h.opLayer, h.opCode);
        const auto ctype_str = cType_string(h.opLayer, h.opCode, h.ctype);

        const std::string con_str = h.con_id.has_value()
                                        ? fmt::format("ConID: {}", *h.con_id)
                                        : "";

        const std::string usn_str = h.usn != 0
                                        ? fmt::format("USN: {}", h.usn)
                                        : "";

        return fmt::format(
          "{}  ├─ {}{}\n"
            "  ├─ {}\n"
            "  ├─ CLS: {}\n"
            "  ├─ AFN: {} [RFN: {} Block#: {}] = (DBA: {:#010x}) {}\n"
            "  ├─ Obj: {} ({:#06x}.{:#06x})\n"
            "  └─ SCN: {} Seq: {} {}",
          coral::Rev_st, desc, coral::Rev_End,
          ctype_str,
          to_string(h.cls),
          h.afn, h.rfile_no, h.block_no, h.dba, usn_str,
          h.obj_id, h.obj_high, h.obj_low,
          toHex(h.scn), h.seq,
          con_str
        );
    }

    static auto decode_afn_obj(uint32_t ao, bool isLittle) noexcept {

       const auto c = coral::decode(ao, isLittle);

       uint16_t afn  = (c      ) & 0xffff;
       uint16_t obj_h =(c >> 16) & 0xffff;

       return std::make_tuple(afn, obj_h);
    }

    static ChangeHead decode( const ChangeHead_lo& c,
                              const std::optional<ChangeHead_ext_lo> ext,
                              const bool isLittle) {

        using coral::decode;

        ChangeHead o;

        o.opLayer = decode(c.opLayer, isLittle);
        o.opCode = decode(c.opCode, isLittle);
        o.seq = decode(c.seq, isLittle);
        o.ctype = decode(c.ctype, isLittle);

        // ----------------------------------------
        const auto [cls_type, usn] = decode_cls(c.cls, isLittle);
        o.cls = cls_type;
        o.usn = usn;

        // ----------------------------------------
        const auto dba = decode(c.dba, isLittle);
        o.dba = dba;
        o.rfile_no =  dba >> 22;           // 10 bit
        o.block_no =  dba & 0x003FFFFFu;   // 22 bit


        // ----------------------------------------
        o.scn = decode_SCN(c.scn, isLittle);

        // ----------------------------------------
        const auto [afn, obj_h] = decode_afn_obj(c.afn_obj, isLittle);
        o.afn       = afn;
        o.obj_high  = obj_h;
        o.obj_low   = decode(c.obj_low, isLittle);
        o.obj_id = (o.obj_high << 16) | o.obj_low;

        // ----------------------------------------
        if (!ext.has_value())
            o.con_id = std::nullopt;
        else
            o.con_id = decode(ext->con_id, isLittle);
        return o;
    }


    ChangeHead ChangeHead_of(const tcb::span<const char> &raw,
                             const bool over12c,
                             const bool isLittle) {


        constexpr auto old_sz = sizeof(ChangeHead_lo);
        constexpr auto ext_sz = sizeof(ChangeHead_ext_lo);

        if (raw.size() < old_sz + (over12c ? ext_sz : 0))
            throw std::out_of_range("not enough data");

        ChangeHead_lo o;
        std::memcpy(&o, raw.data(), old_sz);

        if (!over12c)
            return decode(o, std::nullopt, isLittle);

        ChangeHead_ext_lo e;
        std::memcpy(&e, raw.data() + old_sz , ext_sz);

        return decode(o, e, isLittle);
    }
}
