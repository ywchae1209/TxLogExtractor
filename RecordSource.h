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
        uint32_t last_block_no{0};
        uint32_t last_offset{0};
        uint64_t len;
        std::vector<char> raw;
        RecordHead head;
    };

    inline void show(Record& r, std::ostream &os = std::cout) noexcept {

        using coral::toHex;

        fmt::print(os,
            "R {}.@{}\t~\t{}.@{}\t{}\t",
            r.rba.block_no, r.rba.offset,
            r.last_block_no, r.last_offset,
            r.len
        );
        show(r.head); // << error
    };

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
        std::optional<Block> last_block{std::nullopt};

        Block getOrThrow(std::string_view context = "fill_Records");
        int fill_Records();

    public:
        explicit DefaultRecordSource(std::unique_ptr<BlockSource> bs)
            : block_source(std::move(bs)) {}

        std::optional<Record> getNext() override;
    };


}

