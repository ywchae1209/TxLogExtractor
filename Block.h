#pragma once

#include <vector>
#include <iostream>
#include <tcb/span.hpp>
#include <range/v3/view/transform.hpp>
#include "coral_decode.h"
#include "coral_show.h"

namespace ora {

    // --------------------------------------------------------------------------------
    enum class BHValid {
        Ok = 0,
        Empty,
        Log_sqn_mismatch,
        Error
    };

    inline std::string to_string(BHValid valid) {
        switch (valid) {
            case BHValid::Ok:                return "Ok (0)";
            case BHValid::Empty:             return "Empty(1)";
            case BHValid::Log_sqn_mismatch: return "Log_sqn_mismatch(2)";
            case BHValid::Error:             return "Error(2)";
            default: return "Unknown Error Code (" + std::to_string(static_cast<int>(valid)) + ")";
        }
    }

    // --------------------------------------------------------------------------------
    struct NextRecordOffset {
        uint64_t len;
        uint32_t skip_blocks;  // (0 == 현재 블록)
        uint16_t next_offset;  // 다음 레코드가 시작하는 블록 내 offset
    };

    inline std::string to_string(const NextRecordOffset& p) {
        return fmt::format("{} :> +{}:@{}({})",
                           p.len,
                           p.skip_blocks,
                           p.next_offset,
                           coral::toHex(p.next_offset));
    }

    inline std::string to_string(const std::vector<NextRecordOffset>& offsets) {
        auto mapped = offsets
                      | ranges::views::transform([](auto &p) { return to_string(p); });

        return fmt::format("[{}]", fmt::join(mapped, ", "));
    }


    // --------------------------------------------------------------------------------
    /** redo block head :: block's head in 2nd ~ fin blocks. */
    constexpr int BLOCK_HEADER_LEN = 16;

    struct BlockHead {
        BHValid valid{};
        std::array<uint8_t, 4> signature{};
        uint32_t block_no{};
        uint32_t log_seq_no{};
        uint16_t offset{};   // NOTE :: offset0 & 0x7fff :: first-bit ignored. ( <= 2047 )
        std::array<uint8_t, 2> crc{};
    };

    BlockHead BlockHead_of(const tcb::span<const char> &raw, bool isLittle) noexcept;
    // --------------------------------------------------------------------------------
    struct Block {
        std::vector<char> raw;
        tcb::span<const char> view;
        tcb::span<const char> payload;
        BlockHead head;
        std::vector<NextRecordOffset> offsets;
    };


    Block make_Block(
        std::vector<char> raw,
        uint32_t block_sz,
        bool isLittle,
        const uint32_t head_len = 16 );

    // --------------------------------------------------------------------------------
    void show(const BlockHead &h, std::ostream &os = std::cout) noexcept;
    void show(const BlockHead &h, std::string& suffix, std::ostream &os) noexcept ;
    inline void show(const Block &b, std::ostream &os = std::cout) noexcept {
        auto suffix = to_string(b.offsets);
        show(b.head, suffix, os);
    }
}
