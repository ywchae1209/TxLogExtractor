#pragma once

#include <optional>
#include <queue>

#include "Block.h"
#include "BlockIt.h"

namespace ora {

    // --------------------------------------------------------------------------------
    // --------------------------------------------------------------------------------
    // // min header to iterate
    // struct RecordHeader {
    //     uint32_t len;        // 레코드 전체 길이
    //     uint16_t VLD;        // Valid flag / Opcode 관련 필드
    //     uint16_t scn_wrap;
    //     uint32_t scn_base;
    // };
    //
    // struct Record {
    //     uint32_t len;
    // };
    //
    // class RecordIt {
    //
    //     BlockSource& source;
    //     bool is_little;
    //
    //     std::vector<char>  in_buffer;
    //     std::queue<Record> out_buffer;
    //
    //     bool skip_initial();
    //     std::vector<Record> process_block(const Block& block);
    //
    // public:
    //     explicit RecordIt(BlockSource& src, const bool is_little)
    //         : source(src), is_little(is_little) {}
    //
    //     std::optional<Record> getNext();
    // };
}
