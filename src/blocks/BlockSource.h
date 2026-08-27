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

        virtual const BlockCtx& getCtx() = 0;

        virtual ~BlockSource() = default;
        virtual std::optional<Block> getNext() = 0;
    };

    // ================================================================================
    class FileBlockSource final : public BlockSource {

        FileHead fileHead{};
        RedoHead redoHead{};
        uint32_t log_seq_no{};   //// NOTE :: redoHead->log-file's seq

        uint8_t showBlock;       //// 0: none, 1: Block, 2: bound-candidate

        size_t drain_limit;

        std::ifstream file;
        std::vector<char> read_buf;
        std::deque<Block> out_buffer;

        BlockCtx ctx;

        void drain();

    public:

        const BlockCtx& getCtx() override { return ctx; }

        explicit FileBlockSource(const std::string &path, uint8_t showBlock, size_t buffer_sz = 1);

        [[nodiscard]] const FileHead &getFileHead() const { return fileHead; }
        [[nodiscard]] const RedoHead &getRedoHead() const { return redoHead; }
        [[nodiscard]]std::optional<Block> getNext() override;
    };

}