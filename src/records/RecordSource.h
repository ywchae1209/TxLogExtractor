#pragma once
#include <memory>
#include "../coral_show.h"
#include "../blocks/BlockSource.h"
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

        // local state for getNext
        const BlockCtx ctx;
        uint16_t body_sz;
        std::optional<LwnCtx> lwn_ctx;
        std::optional<Block> latest_block{std::nullopt};

        void update_water(Record& r) {
            if (r.header.lwn) {
                lwn_ctx = (LwnCtx) { r.header.lwn->lwn_start_scn, r.header.lwn->lwn_next_scn };
            }
        }

        [[nodiscard]] Block getOrThrow(std::string_view prefix) const;
        std::optional<Block> get_LatestOrNext();

        int fill_Records();

    public:
        explicit DefaultRecordSource(std::unique_ptr<BlockSource> bs)
            : block_source(std::move(bs))
              , ctx(block_source->getCtx())
              , body_sz(ctx.block_sz - 16) {}

        std::optional<Record> getNext() override;
    };
}

