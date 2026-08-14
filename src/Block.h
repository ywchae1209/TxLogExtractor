#pragma once

#include <vector>
#include <iostream>
#include <tcb/span.hpp>
#include <range/v3/view/transform.hpp>
#include "coral_show.h"
#include "RedoHead.h"

namespace ora {

    // Block validity
    // --------------------------------------------------------------------------------
    enum class BHValid {
        Ok = 0,
        Empty,
        Log_sqn_mismatch,
        Error
    };

    std::string to_string(const BHValid& valid);

    // Block has  record-bounds
    // --------------------------------------------------------------------------------
    struct RecordBound {
        uint32_t len;          // Record length from before-offset to next_offset
        uint32_t next_blocks;  // (0 == 현재 블록)
        uint16_t next_offset;  //
    };

    std::string to_string(const RecordBound& p, size_t block_no);
    std::string to_string(const std::vector<RecordBound>& offsets, size_t block_no);

    // ================================================================================
    /** redo block head :: 2nd block's info */
    struct BlockHead {
        BHValid valid{};
        std::array<uint8_t, 4> signature{};
        uint32_t block_no{};
        uint32_t log_seq_no{};
        uint16_t offset{};   // NOTE :: offset0 & 0x7fff :: first-bit ignored. ( <= 2047 )
        std::array<uint8_t, 2> crc{};

    };

    std::string to_string(const BlockHead &h);

    // ================================================================================
    struct Block {
        std::vector<char> raw;
        BlockHead head;
        std::vector<RecordBound> bounds;

        [[nodiscard]] tcb::span<const char> payload() const {
           return tcb::span(raw).subspan(16);
       }

    };

    std::string to_string(const Block& b);
    void show(const Block& b, std::ostream &os = std::cout);

    constexpr auto SCN_lowest = SCN{0, 0, 0};
    constexpr auto SCN_top = SCN{ .base = 0xFFFFFFFF, .wrap = 0xFFFF, .wrap_high = 0xFFFF };

    Block Block_of(
        std::vector<char> raw,
        uint32_t block_sz,
        bool isLittle,
        const SCN& low = SCN_lowest,
        const SCN& next = SCN_top );

    // --------------------------------------------------------------------------------

}