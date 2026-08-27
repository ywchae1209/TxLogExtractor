#pragma once

#include <vector>
#include <iostream>
#include <tcb/span.hpp>

#include "RecordBound.h"
#include "../coral_show.h"

namespace ora {

    // Block validity
    // --------------------------------------------------------------------------------
    enum class BHValid {
        Ok = 0,
        Empty,
        Log_sqn_mismatch,
        Error
    };

    // --------------------------------------------------------------------------------
    struct BlockHead {
        BHValid valid{};
        std::array<uint8_t, 4> signature{};
        uint32_t block_no{};
        uint32_t log_seq_no{};
        uint16_t offset{};   // NOTE :: offset0 & 0x7fff :: first-bit ignored. ( <= 2047 )
        std::array<uint8_t, 2> crc{};
    };

    // --------------------------------------------------------------------------------
    struct Block {
        std::vector<char> raw;
        BlockHead head;
        std::vector<RecordBound> bounds{};

        [[nodiscard]] tcb::span<const char> payload() const { return tcb::span(raw).subspan(16); }
        uint32_t block_no() const { return head.block_no; };
        uint16_t offset() const { return head.offset; };

    };

    // --------------------------------------------------------------------------------
    constexpr auto BLOCK_HEAD = 16;

    Block Block_of( std::vector<char> raw, bool isLittle);
    Block Block_of( std::vector<char> raw, const BlockCtx& ctx, bool showReason = true );

    std::string to_string(const BoundInfo& p);
    std::string to_string(const RecordBound& p, size_t block_no);
    std::string to_string(const std::vector<RecordBound>& offsets, size_t block_no);

    std::string to_string(const BHValid& valid);
    std::string to_string(const BlockHead &h, bool trim);
    std::string to_string(const Block& b, bool trim = false);
    void show(const Block& b, std::ostream &os = std::cout);
}