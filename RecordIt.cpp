#include "RecordIt.h"
#include "coral_show.h"

namespace ora {

    // --------------------------------------------------------------------------------
    // bool RecordIt::skip_initial() {
    //
    //     while (auto b = source.getNext()) {
    //
    //         const auto& block = b.value();
    //         if (block.head.offset >= 16) {
    //             auto records = process_block(block);
    //             for (auto& rec : records) {
    //                 out_buffer.push(std::move(rec));
    //             }
    //             return true;
    //         }
    //         show(block);  // show skip-block
    //     }
    //     return false; // not-found valid-start-record block
    // }
    //
    //
    // std::vector<Record> RecordIt::process_block(const Block &block) {
    //
    //     std::vector<Record> records;
    //
    //     auto payload = block.payload;
    //     auto offset = block.head.offset;
    //
    //     // -----------------------------------------------------------------
    //     // Case 1: offset == 0 (이전 레코드가 이 블록 전체로 이어짐)
    //     // -----------------------------------------------------------------
    //     if (offset == 0) {
    //         if (in_buffer.empty()) {
    //             show(block);
    //             throw std::runtime_error("Block error: offset == 0 with no-preceding block");
    //         }
    //         in_buffer.insert(in_buffer.end(), payload.begin(), payload.end());
    //         return {};
    //     }
    //
    //     // -----------------------------------------------------------------
    //     // Case 2: offset < 16  ( Block error)
    //     // -----------------------------------------------------------------
    //     if ( offset < 16) {
    //         show(block);
    //         throw std::runtime_error("Block error: offset < 16");
    //     }
    //
    //     // -----------------------------------------------------------------
    //     // Case 2: offset >= 16 (이전 레코드가 offset 직전에서 끝남)
    //     // -----------------------------------------------------------------
    //     if (!in_buffer.empty()) {
    //         size_t cont_bytes = std::min<size_t>(offset, payload.size());
    //         in_buffer.insert(in_buffer.end(), payload.begin(), payload.begin() + cont_bytes);
    //
    //         // 조립된 in_buffer의 레코드 길이 검증 후 추출
    //         auto expected_len = decode_expecting_Record_Length(tcb::span(in_buffer));
    //         if (expected_len && in_buffer.size() >= *expected_len) {
    //             Record rec;
    //             std::memcpy(&rec.head, in_buffer.data(), sizeof(RecordHeader));
    //             rec.raw_data = std::move(in_buffer);
    //             records.push_back(std::move(rec));
    //         }
    //
    //         in_buffer.clear();
    //     }
    //
    //     // -----------------------------------------------------------------
    //     // Case 3: offset 위치부터 현재 블록 내부의 Record(들) 파싱
    //     // -----------------------------------------------------------------
    //     size_t cursor = offset;
    //
    //     while (cursor < payload.size()) {
    //         tcb::span<const char> remain = payload.subspan(cursor);
    //
    //         // RecordHeader 크기조차 남지 않았다면 조각을 in_buffer에 저장하고 다음 블록 대기
    //         if (remain.size() < sizeof(RecordHeader)) {
    //             in_buffer.assign(remain.begin(), remain.end());
    //             break;
    //         }
    //
    //         RecordHeader rec_head;
    //         std::memcpy(&rec_head, remain.data(), sizeof(RecordHeader));
    //
    //         uint32_t rec_len = rec_head.len;
    //         if (rec_len == 0) {
    //             // Padding 영역(0x00) 도달 시 중단
    //             break;
    //         }
    //
    //         // [3-A] 레코드가 현재 블록 안에서 완전히 끝나는 경우
    //         if (remain.size() >= rec_len) {
    //             Record rec;
    //             rec.head = rec_head;
    //             rec.raw_data.assign(remain.begin(), remain.begin() + rec_len);
    //             records.push_back(std::move(rec));
    //
    //             cursor += rec_len; // 다음 레코드로 커서 이동
    //         }
    //         // [3-B] 레코드가 블록 경계를 넘어 다음 블록으로 이어지는 경우
    //         else {
    //             in_buffer.assign(remain.begin(), remain.end());
    //             break;
    //         }
    //     }
    //
    //     return records;
    // }
    //
    // std::optional<Record> RecordIt::getNext() {
    //     return std::nullopt;
    // }
}
