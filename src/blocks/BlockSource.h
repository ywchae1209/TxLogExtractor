#pragma once

#include <deque>
#include <fstream>
#include <optional>
#include <vector>
#include <string>
#include <iterator>

#include "FileHead.h"
#include "RedoHead.h"
#include "Block.h"

namespace ora {

    class BlockSource {
    public :
        virtual const BlockCtx &getCtx() = 0;
        virtual std::optional<Block> getNext() = 0;
        virtual ~BlockSource() = default;
    };

    // ================================================================================
    class FileBlockSource final : public BlockSource {
        FileHead fileHead{};
        RedoHead redoHead{};
        uint32_t log_seq_no{}; //// NOTE :: redoHead->log-file's seq

        uint8_t showMode; //// 0: none, 1: Block, 2: bound-candidate

        std::ifstream file;
        std::vector<char> read_buf;
        std::deque<Block> out_buffer;

        BlockCtx ctx;

        bool drain();

        size_t BLOCKS_PER_READ() {
            const auto fallback = (8 * 1024 * 1024) / ctx.block_sz;
            return (showMode & 2) == 2 ? 1 : fallback;
        }

    public:
        const BlockCtx &getCtx() override { return ctx; }

        explicit FileBlockSource(const std::string &path, uint8_t showMode);

        [[nodiscard]] const FileHead &getFileHead() const { return fileHead; }
        [[nodiscard]] const RedoHead &getRedoHead() const { return redoHead; }

        [[nodiscard]] std::optional<Block> getNext() override;
    };

}