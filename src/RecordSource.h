#pragma once
#include <memory>
#include "coral_show.h"

#include "BlockSource.h"
#include "Record.h"

namespace ora {

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

        Block getOrThrow(std::string_view context);
        std::optional<Block> get_LatestOrNext();

        int fill_Records();

    public:
        bool isLittleEndian;
        bool over12c;
        uint16_t block_sz;
        uint16_t body_sz;

        explicit DefaultRecordSource(std::unique_ptr<BlockSource> bs)
            : block_source(std::move(bs)),
              isLittleEndian(block_source->isLittleEndian()),
              over12c(block_source->over12c()),
              block_sz(block_source->get_Block_sz()),
              body_sz(block_sz - 16) {}

        std::optional<Record> getNext() override;
    };
}

