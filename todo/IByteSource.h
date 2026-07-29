#pragma once

enum class ReadStatus {
    Ok,
    Eof,
    Error     // partial read(깨진 데이터) 또는 I/O 오류
};

struct ReadResult {
    ReadStatus status{ReadStatus::Error};
    size_t size{0};
};

class IByteSource {
public:
    virtual ~IByteSource() = default;
    virtual ReadResult read(char* out, size_t sz) = 0;
    virtual bool skip(size_t sz) = 0;
    virtual bool is_open() = 0;
};
