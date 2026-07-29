#pragma once

#include "IByteSource.h"
#include <functional>
#include <memory>
#include <vector>

enum class GroupReadStatus {
    Ok,
    Rotated,
    Eof,
    Error
};

struct [[nodiscard]] GroupReadResult {
    GroupReadStatus status{GroupReadStatus::Error};
    std::size_t size{0};
};

using SourceOpener = std::function<std::unique_ptr<IByteSource>(std::size_t idx)>;

class SourceGroup {

    SourceOpener opener_;
    std::size_t source_count_{0};
    std::size_t current_idx_{0};
    bool is_circular_{false};

    std::unique_ptr<IByteSource> current_source_{nullptr};

    /**
     * @brief 다음 유효한 소스를 찾아 지연 오픈(Lazy Open) 시도
     */
    bool advance_to_next_source() {
        // 기존 소스 자원 해제 (std::unique_ptr 소멸로 자동 close)
        current_source_.reset();

        std::size_t attempts = 0;
        while (attempts < source_count_) {
            current_idx_++;

            // 인덱스 범위를 넘었을 때 Circular 제어
            if (current_idx_ >= source_count_) {
                if (!is_circular_) {
                    return false; // 모든 소스 소진
                }
                current_idx_ = 0;
            }

            // 지연 오픈 시도
            current_source_ = opener_(current_idx_);
            if (current_source_ && current_source_->is_open()) {
                return true; // 오픈 성공!
            }

            attempts++;
        }

        return false; // 모든 소스 오픈 실패
    }

public:
    SourceGroup(SourceOpener opener, std::size_t source_count, bool is_circular = false)
        : opener_(std::move(opener)), source_count_(source_count), is_circular_(is_circular) {}

    GroupReadResult read(std::byte* out, const std::size_t sz) {

        if (!out || sz == 0 || source_count_ == 0) {
            return {GroupReadStatus::Error, 0};
        }

        // [Step 1] 현재 오픈된 소스가 없으면 첫 번째 소스 지연 오픈 시도
        if (!current_source_ || !current_source_->is_open()) {
            current_source_ = opener_(current_idx_);

            // 첫 번째 소스 오픈 실패 시, 유효한 다음 소스를 찾아 탐색
            if (!current_source_ || !current_source_->is_open()) {
                if (!advance_to_next_source()) {
                    return {GroupReadStatus::Eof, 0};
                }
            }
        }

        // [Step 2] 현재 소스에서 읽기
        const auto [status, bytes_read] = current_source_->read(out, sz);

        if (status == ReadStatus::Ok) { return {GroupReadStatus::Ok, bytes_read}; }
        if (status == ReadStatus::Error) { return {GroupReadStatus::Error, bytes_read}; }

        const bool has_next = advance_to_next_source();

        if (has_next) {
            return {GroupReadStatus::Rotated, 0};
        }

        return {GroupReadStatus::Eof, 0};
    }

    [[nodiscard]] std::size_t current_index() const noexcept {
        return current_idx_;
    }
};