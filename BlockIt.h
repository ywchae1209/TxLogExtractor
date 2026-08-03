#pragma once

#include <deque>
#include <fstream>
#include <iostream>
#include <optional>
#include <vector>
#include <string>
#include <iterator>

#include "FileHead.h"
#include "RedoHead.h"
#include "Block.h"

namespace ora {

    using ora::FileHead, ora::RedoHead, ora::BlockHead, ora::Block;

    // --------------------------------------------------------------------------------
    class BlockSource {
    public :
        virtual bool isLittleEndian() = 0;
        virtual size_t getBlockSize() = 0;

        virtual ~BlockSource() = default;
        virtual std::optional<Block> getNext() = 0;
    };

    // --------------------------------------------------------------------------------
    class BlockIt final : public BlockSource {

        FileHead fileHead{};
        RedoHead redoHead{};
        uint32_t log_seq_no{}; // NOTE :: log-file's seq

        std::streamsize block_sz{};
        std::ifstream file;
        std::vector<char> buf;

        std::deque<Block> buf_drain;
        size_t len_drain;

        void validate(Block&) const noexcept;
        void drain();
        void read_FileHead();
        void read_RedoHead();

    public:
        bool isLittleEndian() override { return fileHead.isLittle; }
        size_t getBlockSize() override { return fileHead.block_sz; }

        explicit BlockIt(const std::string &path, size_t bufferCount = 1);

        [[nodiscard]] const FileHead &getFileHead() const { return fileHead; }
        [[nodiscard]] const RedoHead &getRedoHead() const { return redoHead; }

        [[nodiscard]]std::optional<Block> getNext() override;

        // --------------------------------------------------------------------------------
        // Iterator Subclass
        // --------------------------------------------------------------------------------
        class Iterator {

            BlockIt *parent{nullptr};
            std::optional<Block> current;

        public:
            using iterator_category = std::input_iterator_tag;
            using value_type        = Block;
            using difference_type   = std::ptrdiff_t;
            using pointer           = const Block *;
            using reference         = const Block &;

            Iterator() = default;
            explicit Iterator(BlockIt *p) : parent(p) {
                if (parent) ++(*this);
            }

            const Block &operator*() const { return *current; }
            const Block *operator->() const { return &(*current); }

            Iterator &operator++() {
                if (parent) {
                    current = parent->getNext();
                    if (!current.has_value()) {
                        parent = nullptr; // EOF
                    }
                }
                return *this;
            }

            bool operator==(const Iterator &o) const {
                return (parent == nullptr && o.parent == nullptr) ||
                       (parent == o.parent && current.has_value() == o.current.has_value());
            }

            bool operator!=(const Iterator &o) const {
                return !(*this == o);
            }
        };

        Iterator begin() const { return Iterator(const_cast<BlockIt *>(this)); }
        Iterator end() const { return Iterator(nullptr); }
    };
}