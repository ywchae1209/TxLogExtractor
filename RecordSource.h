#pragma once
#include <memory>

#include "BlockIt.h"
#include "coral_show.h"
#include "RecordHead.h"

namespace ora {

    // ================================================================================
    struct RBA {
        uint32_t log_seq_no{0}; // Redo Log Sequence Number
        uint32_t block_no{0};   // Block Number
        uint16_t offset{0};     // Block 내 Record 시작 Offset
    };

    struct Record {
        RBA rba;
        uint32_t end_block{0};
        uint16_t end_offset{0};
        uint64_t len{0};

        std::vector<char> raw;
        RecordHead head{};
    };

    inline void show(Record& r, bool showDump = false, std::ostream &os = std::cout) noexcept {

        using coral::toHex;

        fmt::println(os,
            "R {}{:>7}.@{:<3} ~{:>7}.@{:<3}{} | "
            "{}",
            coral::bright_green,
            r.rba.block_no, r.rba.offset, r.end_block, r.end_offset,
            coral::reset_color,
            to_string(r.head)
        );

        const auto view = tcb::span<const char>(r.raw);

        if (showDump) {
            coral::show_HexDump(view.subspan(0, r.head.offset));
            coral::show_HexDump(view.subspan(r.head.offset));
        }
    }

    // ================================================================================
    class RecordSource {
    public:
        virtual ~RecordSource() = default;
        virtual std::optional<Record> getNext() = 0;
    };

    // ================================================================================
    class DefaultRecordSource : public RecordSource {

        std::unique_ptr<BlockSource> block_source;
        std::deque<Record> out_buffer;
        std::optional<Block> latest_block{std::nullopt};

        bool isLittleEndian;
        uint16_t block_sz;
        Block getOrThrow(std::string_view context);
        std::optional<Block> get_LatestOrNext();

        int fill_Records();

    public:
        explicit DefaultRecordSource(std::unique_ptr<BlockSource> bs)
            : block_source(std::move(bs)),
              block_sz(block_source->get_Block_sz()),
              isLittleEndian(block_source->isLittleEndian()) {}

        std::optional<Record> getNext() override;
    };
}

