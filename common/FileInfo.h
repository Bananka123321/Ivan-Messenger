#pragma once

#include <cstdint>
#include <string>

#include "Message.h"

// Публичные метаданные файла: то, что видят участники диалога.
struct FileInfo {
    int64_t file_id = 0;
    std::string filename;
    std::string mime_type;
    MessageType kind = MessageType::File;
    uint64_t size_bytes = 0;
    std::string sha256;     // hex, 64 символа в нижнем регистре
};
