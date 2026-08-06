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

    // --------------------------------------------------------------------------------
    class BlockSource {
    public :
        virtual bool isLittleEndian() = 0;
        virtual uint16_t get_Block_sz() = 0;

        virtual ~BlockSource() = default;
        virtual std::optional<Block> getNext() = 0;
    };

    // --------------------------------------------------------------------------------
    class BlockIt final : public BlockSource {

        FileHead fileHead{};
        RedoHead redoHead{};
        uint32_t log_seq_no{}; // NOTE :: redoHead->log-file's seq

        bool showBlock;
        bool showDump;

        size_t out_buffer_sz;

        std::ifstream file;
        std::vector<char> read_buf;

        std::deque<Block> out_buffer;

        void drain();

    public:
        bool isLittleEndian() override { return fileHead.isLittle; }
        uint16_t get_Block_sz() override { return fileHead.block_sz; }

        explicit BlockIt(const std::string &path, bool showBlock = true, size_t buffer_sz = 1);

        [[nodiscard]] const FileHead &getFileHead() const { return fileHead; }
        [[nodiscard]] const RedoHead &getRedoHead() const { return redoHead; }

        [[nodiscard]]std::optional<Block> getNext() override;
    };
}