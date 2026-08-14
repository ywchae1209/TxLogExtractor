#include "Block.h"
#include "coral_decode.h"
#include "coral_record.h"
#include "coral_show.h"
#include <iostream>

namespace ora {
    // --------------------------------------------------------------------------------

    std::string to_string(
        const BHValid &valid
    ) {
        switch (valid) {
            case BHValid::Ok: return "";
            case BHValid::Empty: return "Empty(1)";
            case BHValid::Log_sqn_mismatch: return "Log_sqn_mismatch(2)";
            case BHValid::Error: return "Error(2)";
            default: return "Unknown Error Code (" + std::to_string(static_cast<int>(valid)) + ")";
        }
    }

    // --------------------------------------------------------------------------------
    std::string to_string(
        const RecordBound &p,
        const size_t block_no
    ) {

        using coral::toHex;
        auto next_blocks = block_no + p.next_blocks;
        return fmt::format("{} :> {}{}.@{}{}({})", p.len,
                           coral::key_color,
                           next_blocks, p.next_offset,
                           coral::reset_color,
                           toHex(p.next_offset));
    }

    std::string to_string(
        const std::vector<RecordBound> &offsets,
        const size_t block_no
    ) {

        auto mapped = offsets
                      | ranges::views::transform([block_no](auto &p) { return to_string(p, block_no); });

        return fmt::format("[{}]", fmt::join(mapped, ", "));
    }

    // --------------------------------------------------------------------------------

    std::string to_string(const BlockHead &h) {
        using coral::toHex;
        return fmt::format(
            "#{:<7} LSN:{}({}) Sig:{} off= {:<3}({}) {}",
            h.block_no,
            h.log_seq_no, toHex(h.log_seq_no),
            toHex(h.signature),
            h.offset, toHex(h.offset),
            to_string(h.valid));
    }

    // --------------------------------------------------------------------------------

    constexpr auto BLOCK_HEAD_LEN = 16;

#pragma pack(push, 1)
    struct BlockHead_lo {
        uint8_t signature[4]; // signature = Block Type 0x2201
        uint32_t block_no;
        uint32_t log_seq_no;
        uint16_t offset0; // offset = record-begin offset
        uint8_t crc[2]; // crc
    };
#pragma pack(pop)

    static BlockHead decode(const BlockHead_lo &raw, const bool isLittle) noexcept {
        const uint16_t offset = coral::decode(raw.offset0, isLittle) & 0x7fff;

        BlockHead h{};

        h.valid = BHValid::Ok;

        h.signature[0] = raw.signature[0];
        h.signature[1] = raw.signature[1];
        h.signature[2] = raw.signature[2];
        h.signature[3] = raw.signature[3];

        h.block_no = coral::decode(raw.block_no, isLittle);
        h.log_seq_no = coral::decode(raw.log_seq_no, isLittle);

        h.offset = offset;

        h.crc[0] = raw.crc[0];
        h.crc[1] = raw.crc[1];

        return h;
    }

    static BlockHead BlockHead_of(
        const tcb::span<const char> &raw,
        const bool isLittle
    ) noexcept {
        if (raw.size() < sizeof(BlockHead_lo))
            return BlockHead{.valid = BHValid::Empty};

        const auto lo = reinterpret_cast<const BlockHead_lo *>(raw.data());
        return decode(*lo, isLittle);
    }

    static std::optional<RecordBound> calculate_next(
        const uint32_t offset,
        const uint32_t record_len,
        const uint32_t block_sz = 512) {
        const uint32_t room = block_sz - 16; // 512 - 16 = 496
        const uint32_t room0 = block_sz - offset; // current room

        const int32_t needs = record_len - room0;

        if (needs < 0) return RecordBound{record_len, 0, static_cast<uint16_t>(offset + record_len)};
        if (needs == 0) return RecordBound{record_len, 1, 16};

        const uint32_t blocks = (needs / room) + 1; // 몫
        const uint16_t remain = needs % room; // 나머지

        return (remain == 0)
                   ? RecordBound{record_len, blocks, 16}
                   : RecordBound{record_len, blocks, static_cast<uint16_t>(16 + remain)};
    }

    static std::vector<RecordBound> calculate_bounds(
        const tcb::span<const char> view,
        const uint16_t offset0,
        const uint32_t block_sz,
        const bool isLittle,
        const SCN &low,
        const SCN &next
    ) {

        std::vector<RecordBound> result;

        auto offset = offset0;
        std::optional<uint32_t> len0 = coral::read_record_length_with_validation(view, offset, isLittle, low, next);

        int limit = 0;
        while (len0 && limit < 32) {
            if (*len0 == 0) break;

            limit++;
            auto n = calculate_next(offset, *len0, block_sz);
            if (!n) break;

            auto bound = n.value();

            result.push_back(bound);

            if (bound.next_blocks > 0) break;

            offset = bound.next_offset;
            len0 = coral::read_record_length_with_validation(view, offset, isLittle, low, next);
        }

        return result;
    }

    std::string to_string(
        const Block &b
    ) {

        return fmt::format("B {} {}",
            to_string(b.head),
            to_string(b.bounds, b.head.block_no));
    }

    void show(
        const Block &b,
        std::ostream &os
    ) {
        fmt::println(os, "{}", to_string(b));
    }

    Block Block_of (
        std::vector<char> raw,
        const uint32_t block_sz,
        const bool isLittle,
        const SCN &low,
        const SCN &next)
    {

        const auto head = BlockHead_of(raw, isLittle);

        Block block{};

        block.raw = std::move(raw);
        block.head = head;
        block.bounds = calculate_bounds(block.raw, head.offset, block_sz, isLittle, low, next);

        return block;
    }
}
