#pragma once

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

    // 24
    struct ChangeHead_base_lo {
        uint8_t opLayer;        /// [5: Undo/Tx, 10: Index, 11: DataBlock...]
        uint8_t opCode;

        uint16_t cls;           /// * Block-Class ID ~ may contain usn(undo segment number)
        uint32_t afn_obj;       /// * AFN (low 2) + boj_high( High 2)
        uint32_t dba;           /// * Data Block Address

        SCN scn;                /// ignore wrap-high

        uint8_t seq;            /// Change Sequence Number : 동일한 Redo Record 또는 동일 SCN 내에서 해당 체인지 벡터의 순번
        uint8_t ctype;          /// * Change Type
        uint16_t obj_low;       /// * Data-Object-Id-Low
    };

    // 8
    struct ChangeHead_ext_lo {
        uint8_t con_id;
        uint8_t flag[3];
        uint32_t aux;
    };

    struct ChangeHead_lo {
        ChangeHead_base_lo base;
        std::optional<ChangeHead_ext_lo> ext;
    };


#pragma pack(pop)

    template <bool IsLittle>
    inline ChangeHead_base_lo decode_change_head_base0(tcb::span<const char> buf) noexcept {

        using coral::decode_at;

        return ChangeHead_base_lo{
            decode_at<uint8_t, IsLittle>(buf, 0),   // opLayer
            decode_at<uint8_t, IsLittle>(buf, 1),   // opCode
            decode_at<uint16_t,IsLittle>(buf, 2),   // cls
            decode_at<uint32_t,IsLittle>(buf, 4),   // afn_obj
            decode_at<uint32_t,IsLittle>(buf, 8),   // dba
            decode_scn0s_at   <IsLittle>(buf, 12),  // scn (SCN: 8 bytes)
            decode_at<uint8_t, IsLittle>(buf, 20),  // seq
            decode_at<uint8_t, IsLittle>(buf, 21),  // ctype
            decode_at<uint16_t,IsLittle>(buf, 22)   // obj_low
        };
    }

    template <bool IsLittle>
    inline ChangeHead_ext_lo decode_change_head_ext0(tcb::span<const char> buf) noexcept {
        using coral::decode_at;

        ChangeHead_ext_lo ext{};

        ext.con_id = decode_at<uint8_t, IsLittle>(buf, 0);

        const uint8_t* ptr = reinterpret_cast<const uint8_t*>(buf.data() + 1);
        ext.flag[0] = ptr[0];
        ext.flag[1] = ptr[1];
        ext.flag[2] = ptr[2];

        ext.aux = decode_at<uint32_t, IsLittle>(buf, 4);
        return ext;
    }


    inline ChangeHead_base_lo decode_change_head_base(tcb::span<const char> buf, bool isLittle) {
        return isLittle
                   ? decode_change_head_base0<true>(buf)
                   : decode_change_head_base0<false>(buf);
    }

    inline ChangeHead_ext_lo decode_change_head_ext(tcb::span<const char> buf, bool isLittle) {
        return isLittle
                   ? decode_change_head_ext0<true>(buf)
                   : decode_change_head_ext0<false>(buf);
    }

    inline ChangeHead_lo decode_change_head(tcb::span<const char> buf, bool hasExt, bool isLittle) {
        // caller must check buf-size

        return ChangeHead_lo{
            decode_change_head_base(buf, isLittle),
            hasExt
                ? std::make_optional(decode_change_head_ext(buf, isLittle))
                : std::nullopt
        };
    }
}