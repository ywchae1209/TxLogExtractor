#pragma once

#include "Change.h"
#include "../coral_decode.h"
#include "../ora_layout.h"

namespace ora {
    /// https://renenyffenegger.ch/notes/index.html
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
        uint8_t opLayer;        /// [5: Undo/Tx, 10: Index, 11: DataBlock...]
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

    inline static auto decode_cls(const uint16_t c0, const bool isLittle) noexcept {

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

    inline static auto decode_afn_obj(const uint32_t ao, const bool isLittle) noexcept {

        const auto c = coral::decode(ao, isLittle);

        uint16_t afn  = (c      ) & 0xffff;
        uint16_t obj_h =(c >> 16) & 0xffff;

        return std::make_tuple(afn, obj_h);
    }

    inline static auto decode(const ChangeHead_lo &ch,
                              const std::optional<ChangeHead_ext_lo> &ch_ext,
                              const tcb::span<const char> span,
                              const bool isLittle) -> ChangeHead {
        using coral::decode;

        ChangeHead o;

        o.size = span.size();
        o.span = span;

        o.opLayer = decode(ch.opLayer, isLittle);
        o.opCode = decode(ch.opCode, isLittle);
        o.seq = decode(ch.seq, isLittle);
        o.ctype = decode(ch.ctype, isLittle);

        // ----------------------------------------
        const auto [cls_type, usn] = decode_cls(ch.cls, isLittle);
        o.cls = cls_type;
        o.usn = usn;

        // ----------------------------------------
        const auto dba = decode(ch.dba, isLittle);
        o.dba = dba;
        o.rfile_no =  dba >> 22;           // 10 bit
        o.block_no =  dba & 0x003FFFFFu;   // 22 bit

        // ----------------------------------------
        o.scn = decode_SCN(ch.scn, isLittle);

        // ----------------------------------------
        const auto [afn, obj_h] = decode_afn_obj(ch.afn_obj, isLittle);
        o.afn       = afn;
        o.obj_high  = obj_h;
        o.obj_low   = decode(ch.obj_low, isLittle);
        o.obj_id    = (o.obj_high << 16) | o.obj_low;

        // ----------------------------------------
        o.con_id = ch_ext ? std::make_optional(decode(ch_ext->con_id, isLittle)) : std::nullopt;

        return o;
    }
}