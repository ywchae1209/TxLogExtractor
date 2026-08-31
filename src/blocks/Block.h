#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <tcb/span.hpp>

#include "RecordBound.h"
#include "../coral_show.h"

namespace ora {

    // --------------------------------------------------------------------------------
    struct BlockHead {
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

        void set_head(bool isLittle);
        void set_bounds(const BlockCtx& ctx, bool showReason);

        [[nodiscard]] tcb::span<const char> payload() const {
            if (raw.size() <= 16) return {};
            return tcb::span(raw.data(), raw.size()).subspan(16);
        }

        [[nodiscard]] tcb::span<const char> raw_span() const {
            return {raw.data(), raw.size()};
        }

        [[nodiscard]] uint32_t block_no() const { return head.block_no; };
        [[nodiscard]] uint16_t offset() const { return head.offset; };
    };

    // --------------------------------------------------------------------------------
    constexpr auto BLOCK_HEAD = 16;

    Block Block_of(tcb::span<const char> raw, const bool isLittle);

    std::string to_string(const BoundInfo& p);
    std::string to_string(const RecordBound& p, size_t block_no);
    std::string to_string(const std::vector<RecordBound>& offsets, size_t block_no);

    // std::string to_string(const BHValid& valid);
    std::string to_string(const BlockHead &h, bool trim = true);
    std::string to_string(const Block& b, bool trim = false);
    void show(const Block& b, const bool dump= false, std::ostream &os = std::cout);
}