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

// ring buffer
#include <vector>
#include <optional>
#include <cstddef>
#include <utility>
#include <stdexcept>

namespace ora {

    template<typename T, size_t Capacity>
    class RingBuffer {
        std::vector<T> buffer_;
        size_t head_{0}; // Pop(읽기) 위치
        size_t tail_{0}; // Push(쓰기) 위치
        size_t size_{0}; // 현재 담긴 원소 수

    public:
        RingBuffer() {
            buffer_.resize(Capacity);
        }

        [[nodiscard]] bool empty() const noexcept {
            return size_ == 0;
        }

        [[nodiscard]] bool full() const noexcept {
            return size_ == Capacity;
        }

        [[nodiscard]] size_t size() const noexcept {
            return size_;
        }

        [[nodiscard]] static constexpr size_t capacity() noexcept {
            return Capacity;
        }

        bool push(T &&value) noexcept {
            if (full()) return false;

            buffer_[tail_] = std::move(value);
            tail_ = (tail_ + 1) % Capacity;
            ++size_;
            return true;
        }

        template<typename... Args>
        bool emplace(Args &&... args) noexcept {
            if (full()) return false;

            buffer_[tail_] = T(std::forward<Args>(args)...);
            tail_ = (tail_ + 1) % Capacity;
            ++size_;
            return true;
        }

        std::optional<T> pop() noexcept {
            if (empty()) return std::nullopt;

            T val = std::move(buffer_[head_]);
            head_ = (head_ + 1) % Capacity;
            --size_;
            return val;
        }

        bool pop(T &out_val) noexcept {
            if (empty()) return false;

            out_val = std::move(buffer_[head_]);
            head_ = (head_ + 1) % Capacity;
            --size_;
            return true;
        }

        [[nodiscard]] const T *front_ptr() const noexcept {
            if (empty()) return nullptr;
            return &buffer_[head_];
        }

        void advance_head() noexcept {
            if (!empty()) {
                head_ = (head_ + 1) % Capacity;
                --size_;
            }
        }

        void clear() noexcept {
            head_ = 0;
            tail_ = 0;
            size_ = 0;
        }
    };


    // ================================================================================

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