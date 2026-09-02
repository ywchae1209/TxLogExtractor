
#pragma once

#include <vector>
#include <optional>
#include <string>

#include "tcb/span.hpp"
#include "../ora_layout.h"
#include "../coral_result.h"

namespace ora {

    // --------------------------------------------------------------------------------
    enum class BlockClassType {
        Unknown = 0,           ///
        DataBlock,             ///  1: Data block         - 일반 테이블/인덱스 데이터 블록
        SortBlock,             ///  2: Sort block         - 정렬(Sort) 작업용 임시 블록
        SaveUndoBlock,         ///  3: Save undo block    - Savepoint 적용 시 생성되는 Save Undo 데이터 블록
        SegmentHeader,         ///  4: Segment header     - 세그먼트(테이블/인덱스 등) 헤더 블록
        SaveUndoHeader,        ///  5: Save undo header   - Savepoint용 Save Undo 헤더 블록
        FreeListBlock,         ///  6: Free list block    - 수동 공간 관리(MSSM) 방식의 Free List 블록
        ExtentMapBlock,        ///  7: Extent map block   - 익스텐트 맵 관리 블록
        Bmb1st,                ///  8: 1st level BMB      - ASSM 1단계 비트맵
        Bmb2nd,                ///  9: 2nd level BMB      - ASSM 2단계 비트맵
        Bmb3rd,                /// 10: 3rd level BMB      - ASSM 3단계 비트맵
        BitmapBlock,           /// 11: Bitmap block       - 일반 비트맵 관리 블록
        BitmapIndexBlock,      /// 12: Bitmap index block - 비트맵 인덱스 데이터 블록
        FileHeaderBlock,       /// 13: File header block  - 데이터파일 헤더 블록
        DeferredRollbackBlock, /// 14: Deferred rollback  - 지연 롤백 세그먼트 블록
        SystemUndoHeader,      /// 15: SYSTEM undo header - SYSTEM Undo 세그먼트 헤더 (USN 0)
        SystemUndoBlock,       /// 16: SYSTEM undo block  - SYSTEM Undo 데이터 블록 (USN 0)
        UndoHeader,            /// 17, 19...: Undo header - 일반 Undo 세그먼트 헤더 (USN >= 1)
        UndoBlock,             /// 18, 20...: Undo block  - 일반 Undo 데이터 블록 (USN >= 1)
    };

    inline static auto get_cls_usn(const uint16_t c) noexcept {

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


    // --------------------------------------------------------------------------------
    struct ChangeHead {

        tcb::span<const char> span;

        size_t size;

        uint8_t  opLayer;
        uint8_t  opCode;

        BlockClassType cls;
        uint16_t usn;                   //// undo segment number ( CLS >= 15)

        uint16_t afn;                   //// DB전체에서 유일한 데이터 파일번호

        uint32_t obj_id;                //// 변경대상 object 판정용 ( truncate되면 변경됨)
        uint16_t obj_low;
        uint16_t obj_high;

        uint32_t dba;                   //// Data Block Address
        uint16_t rfile_no;              //// 테이블 스페이스 내의 파일번호(10비트)
        uint32_t block_no;              ///

        SCN      scn;                   //// ignore-wrap-high
        uint8_t  seq;                   ////

        uint8_t  ctype;                 //// Change Type

        std::optional<uint8_t>  con_id; //// Container ID
    };

    // --------------------------------------------------------------------------------
    struct LengthVector {
        const std::vector<uint16_t> sizes;
        const std::vector<tcb::span<const char>> spans;
    };

    // --------------------------------------------------------------------------------
    struct Change {
        ChangeHead change_head;
        LengthVector length_vector;
    };

    // --------------------------------------------------------------------------------
    auto Changes_of(
        const RBA &rba,
        const tcb::span<const char> &raw,
        bool over12c,
        bool isLittle, std::vector<Change>&) -> coral::Result<bool>;

    // --------------------------------------------------------------------------------
    std::string to_string(BlockClassType cls);
    std::string to_string(const ChangeHead& h);

    void show(const LengthVector& lv, bool dump);

}
