#pragma once

#include <vector>
#include <iostream>
#include <memory>
#include <tcb/span.hpp>

#include "layout_BlockHead.h"
#include "RecordBound.h"
#include "../coral_show.h"

namespace ora {

    struct Block {
        std::vector<char> raw;

        BlockHead head;
        std::vector<RecordBound> bounds{};

        void set_head(bool isLittle);
        void set_bounds(const BlockCtx& ctx, bool showReason);

        [[nodiscard]] tcb::span<const char> raw_span() const { return {raw.data(), raw.size()}; }
        [[nodiscard]] uint32_t log_seq_no() const { return head.log_seq_no; };
        [[nodiscard]] uint32_t block_no() const { return head.block_no; };
        [[nodiscard]] uint16_t offset() const { return head.offset; };
        [[nodiscard]] tcb::span<const char> payload() const {
            return (raw.size() <= 16)
                       ? tcb::span<const char>{}
                       : tcb::span(raw.data(), raw.size()).subspan(16);
        }

        bool isEmpty() const { return raw.size() == 0; }
    };

    // --------------------------------------------------------------------------------
    constexpr auto BLOCK_HEAD = 16;

    inline Block Block_of(tcb::span<const char> raw, const bool isLittle) {
        return Block{
            .raw  = std::vector(raw.begin(), raw.end()),
            .head = decode_BlockHead(raw, isLittle)
        };
    }

    std::string to_string(const BoundInfo& p);
    std::string to_string(const RecordBound& p, size_t block_no);
    std::string to_string(const std::vector<RecordBound>& offsets, size_t block_no);

    std::string to_string(const BlockHead &h, bool trim = true);
    std::string to_string(const Block& b, bool trim = false);
    void show(const Block& b, bool dump= false, std::ostream &os = std::cout);
}