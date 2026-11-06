#pragma once

#include <array>
#include <mutex>
#include <string>
#include <string_view>

#include "DB_DialogManager.h"
#include "DB_FileManager.h"
#include "FileStorage.h"

struct UploadRequest {
    int owner_id = 0;
    int64_t dialog_id = 0;
    std::string filename;
    std::string mime_type;
    MessageType kind = MessageType::File;
    uint64_t size_bytes = 0;
    std::string sha256;
};

struct UploadProgress {
    bool ok = false;
    std::string error;
    int64_t file_id = 0;
    uint64_t offset = 0;        // сколько байт сервер уже принял
    bool completed = false;
    FileStatus status = FileStatus::Uploading;
};

struct DownloadChunk {
    bool ok = false;
    std::string error;
    FileInfo file;
    uint64_t offset = 0;
    std::string data;
};

struct AttachmentCheck {
    bool ok = false;
    std::string error;
    FileInfo file;
};

// Логика обмена файлами. Handler разбирает пакеты и проверяет авторизацию,
// а здесь проверяются права на файл, порядок чанков и целостность.
class FileTransferService {
public:
    FileTransferService(DB_ConnectionPool& pool, FileStorage& storage);

    UploadProgress initUpload(const UploadRequest& request);
    UploadProgress uploadStatus(int user_id, int64_t file_id);
    UploadProgress writeChunk(int user_id, int64_t file_id, uint64_t offset, std::string_view data);

    DownloadChunk readChunk(int user_id, int64_t file_id, uint64_t offset);

    // Можно ли прикрепить файл к сообщению: файл загружен целиком, этим же пользователем и в этот же диалог
    AttachmentCheck checkAttachment(int user_id, int64_t file_id, int64_t dialog_id, MessageType type);

private:
    DB_FileManager files_;
    DB_DialogManager dialogs_;
    FileStorage& storage_;

    // Чанки одного файла обрабатываются строго по очереди, разные файлы — параллельно.
    // Пользователь может грузить файл с двух устройств одновременно, поэтому нужна блокировка.
    std::array<std::mutex, 64> locks_;
    std::mutex& lockFor(int64_t file_id);

    bool isParticipant(int user_id, int64_t dialog_id);
};
