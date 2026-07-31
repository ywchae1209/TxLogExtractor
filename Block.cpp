#include "Block.h"
#include "coral_decode.h"
#include "coral_record.h"
#include "coral_show.h"

namespace ora {

#pragma pack(push, 1)
    struct BlockHead_lo {
        uint8_t signature[4];   // signature = Block Type 0x2201
        uint32_t block_no;
        uint32_t log_seq_no;
        uint16_t offset0;       // offset = record-begin offset
        uint8_t crc[2];         // crc
    };
#pragma pack(pop)

    BlockHead decode(const BlockHead_lo &raw, const bool isLittle) noexcept {

        const uint16_t offset = coral::decode(raw.offset0, isLittle) & 0x7fff;

        BlockHead h{};

        h.valid = BHValid::Ok;

        h.signature[0] = raw.signature[0];
        h.signature[1] = raw.signature[1];
        h.signature[2] = raw.signature[2];
        h.signature[3] = raw.signature[3];

        h.block_no   = coral::decode(raw.block_no, isLittle);
        h.log_seq_no = coral::decode(raw.log_seq_no, isLittle);

        h.offset = offset;

        h.crc[0] = raw.crc[0];
        h.crc[1] = raw.crc[1];

        return h;
    }

    BlockHead BlockHead_of(const tcb::span<const char> &raw, const bool isLittle) noexcept {
        if (raw.size() < sizeof(BlockHead_lo))
            return BlockHead{ .valid = BHValid::Empty};

        const auto lo = reinterpret_cast<const BlockHead_lo*>(raw.data());
        return decode(*lo, isLittle);
    }

    std::optional<NextRecordOffset> calculate_next(
        const uint32_t offset,
        const uint32_t record_len,
        const uint32_t block_sz = 512)
    {

        const uint32_t room = block_sz - BLOCK_HEAD_LEN; // 512 - 16 = 496
        const uint32_t room0 = block_sz - offset;  // current room

        const int32_t needs = record_len - room0;

        if (needs < 0) return  NextRecordOffset{record_len, 0, static_cast<uint16_t>(offset + record_len)};
        if (needs == 0) return NextRecordOffset{record_len, 1, BLOCK_HEAD_LEN};

        const uint32_t blocks = needs / room; // 몫
        const uint16_t remain = needs % room; // 나머지

        return (remain == 0)
                   ? NextRecordOffset{record_len, blocks, BLOCK_HEAD_LEN}
                   : NextRecordOffset{record_len, blocks + 1, static_cast<uint16_t>(BLOCK_HEAD_LEN + remain)};
    }

    std::vector<NextRecordOffset> next_offsets(
        const tcb::span<const char> view,
        const uint16_t offset0,
        const uint32_t block_sz,
        const bool isLittle,
        const SCN& low,
        const SCN& next ) {

        std::vector<NextRecordOffset> result;

        auto offset = offset0;
        std::optional<uint32_t> len0 = coral::read_record_length_with_validation(view, offset, isLittle, low, next);

        int limit = 0;
        while (len0 && limit < 32) {
            if (*len0 == 0 ) break;

            limit++;
            auto n = calculate_next(offset, *len0, block_sz);
            if (!n) break;

            auto next_start = n.value();

            result.push_back(next_start);

            if (next_start.skip_blocks > 0) break;

            offset = next_start.next_offset;
            len0 = coral::read_record_length_with_validation(view, offset, isLittle, low, next);

        }

        return result;
    }

    Block make_Block(
        std::vector<char> raw,
        const uint32_t block_sz,
        const bool isLittle,
        const SCN& low,
        const SCN& next ) {

        const auto head = BlockHead_of(raw, isLittle);

        Block block {};

        block.raw = std::move(raw);
        block.view = tcb::span<const char>(block.raw);
        block.payload = block.view.subspan(16);
        block.head = head;
        block.offsets = next_offsets(block.view, head.offset, block_sz, isLittle, low, next);

        return block;
    }

    void show(const BlockHead &h, std::string& suffix, std::ostream &os) noexcept {

        using coral::toHex;

        fmt::print(os,
            "#{}\tLSQN {}({})\tSIG:{} offset: {} ({}) {} {}\n",
            h.block_no,
            h.log_seq_no, toHex(h.log_seq_no),
            toHex(h.signature),
            toHex(h.offset), h.offset,
            h.valid == BHValid::Ok ? "" : to_string(h.valid), suffix
        );
    }

    void show(const BlockHead &h, std::ostream &os) noexcept{
        using coral::toHex;

        std::string empty = "";
        show(h, empty, os);
    }

    // --------------------------------------------------------------------------------
}
