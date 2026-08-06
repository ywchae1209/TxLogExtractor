#pragma once

#include <vector>
#include <iostream>
#include <tcb/span.hpp>
#include <range/v3/view/transform.hpp>
#include "coral_show.h"
#include "RedoHead.h"

namespace ora {

    // --------------------------------------------------------------------------------
    enum class BHValid {
        Ok = 0,
        Empty,
        Log_sqn_mismatch,
        Error
    };

    inline std::string to_string(const BHValid& valid) {
        switch (valid) {
            case BHValid::Ok:                return "";
            case BHValid::Empty:             return "Empty(1)";
            case BHValid::Log_sqn_mismatch: return "Log_sqn_mismatch(2)";
            case BHValid::Error:             return "Error(2)";
            default: return "Unknown Error Code (" + std::to_string(static_cast<int>(valid)) + ")";
        }
    }

    // --------------------------------------------------------------------------------
    struct RecordBound {
        uint64_t len;          // Record length from beforer-offset to next_offset
        uint32_t next_blocks;  // (0 == 현재 블록)
        uint16_t next_offset;  // 다음 레코드가 시작하는 블록 내 offset
    };

    inline std::string to_string(const RecordBound& p) {
        using coral::toHex;
        return p.next_blocks == 0
                   ? fmt::format("{} :> @{}({})", p.len, p.next_offset, toHex(p.next_offset))
                   : fmt::format("{} :> +{}.@{}({})", p.len, p.next_blocks, p.next_offset, toHex(p.next_offset));
    }

    inline std::string to_string(const RecordBound& p, const size_t block_no) {
        using coral::toHex;
        auto next_blocks = block_no + p.next_blocks;
        return fmt::format("{} :> {}{}.@{}{}({})", p.len,
                           coral::bright_green,
                           next_blocks, p.next_offset,
                           coral::reset_color,
                           toHex(p.next_offset));
    }

    inline std::string to_string(const std::vector<RecordBound>& offsets) {
        auto mapped = offsets
                      | ranges::views::transform([](auto &p) { return to_string(p); });

        return fmt::format("[{}]", fmt::join(mapped, ", "));
    }

    inline std::string to_string(const std::vector<RecordBound>& offsets, const size_t block_no) {
        auto mapped = offsets
                      | ranges::views::transform([block_no](auto &p) { return to_string(p, block_no); });

        return fmt::format("[{}]", fmt::join(mapped, ", "));
    }

    // ================================================================================
    /** redo block head :: block's head in 2nd ~ fin blocks. */
    struct BlockHead {
        BHValid valid{};
        std::array<uint8_t, 4> signature{};
        uint32_t block_no{};
        uint32_t log_seq_no{};
        uint16_t offset{};   // NOTE :: offset0 & 0x7fff :: first-bit ignored. ( <= 2047 )
        std::array<uint8_t, 2> crc{};

    };

    inline std::string to_string(const BlockHead &h){

        using coral::toHex;
        return fmt::format(
            "#{:<7} LSN:{}({}) Sig:{} off= {:<3}({}) {}",
            h.block_no,
            h.log_seq_no, toHex(h.log_seq_no),
            toHex(h.signature),
            h.offset, toHex(h.offset),
            to_string(h.valid));
    }

    // ================================================================================
    struct Block {
        std::vector<char> raw;
        tcb::span<const char> view;
        tcb::span<const char> payload;

        BlockHead head;
        std::vector<RecordBound> bounds;

        bool isOk() const noexcept { return head.valid ==  BHValid::Ok; }
    };

    inline std::string to_string(const Block& b) noexcept {
        return fmt::format("B {} {}", to_string(b.head), to_string(b.bounds, b.head.block_no) );
    }

    inline void show(const Block& b, std::ostream &os = std::cout) noexcept {
        fmt::println(os, "{}", to_string(b) );
    }

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