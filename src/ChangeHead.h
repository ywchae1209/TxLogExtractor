#pragma once

#include <optional>
namespace ora {

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

    std::string to_string(BlockClassType cls);

    struct ChangeHead {

        uint8_t  opLayer;
        uint8_t  opCode;

        BlockClassType cls;
        uint16_t usn;                   //// undo segment number ( CLS >= 15)

        // --------------------------------------------------------------------------------
        uint16_t afn;                   //// DB전체에서 유일한 데이터 파일번호

        // --------------------------------------------------------------------------------
        uint32_t obj_id;                //// 변경대상 object 판정용 ( truncate되면 변경됨)
        uint16_t obj_low;
        uint16_t obj_high;


        // --------------------------------------------------------------------------------
        uint32_t dba;                   //// Data Block Address
        uint16_t rfile_no;              //// 테이블 스페이스 내의 파일번호(10비트)
        uint32_t block_no;              ///

        // --------------------------------------------------------------------------------
        SCN      scn;                   //// ignore-wrap-high
        uint8_t  seq;                   ////

        // --------------------------------------------------------------------------------
        uint8_t  ctype;                 //// Change Type
        std::optional<uint8_t>  con_id; //// Container ID
    };

    std::string to_string(ChangeHead& h);

    ChangeHead ChangeHead_of(const tcb::span<const char> &raw, bool over12c, bool isLittle);
}
