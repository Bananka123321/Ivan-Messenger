#pragma once

#include <optional>
#include <string>

#include "DB_ConnectionPool.h"
#include "FileInfo.h"

enum class FileStatus {
    Uploading,
    Complete,
    Failed
};

std::string toString(FileStatus status);

// Строка таблицы files: публичные метаданные + служебные поля сервера.
struct FileRecord {
    FileInfo info;
    int owner_id = 0;
    int64_t dialog_id = 0;
    std::string storage_name;
    uint64_t uploaded_bytes = 0;
    FileStatus status = FileStatus::Uploading;
};

class DB_FileManager {
public:
    explicit DB_FileManager(DB_ConnectionPool& pool);

    // Создаёт запись со статусом uploading и возвращает её id
    std::optional<int64_t> create(const FileRecord& file);

    std::optional<FileRecord> get(int64_t file_id);

    // Незавершённая загрузка того же файла тем же пользователем в тот же диалог.
    // Позволяет продолжить загрузку после перезапуска клиента, даже если он потерял file_id.
    std::optional<FileRecord> findResumable(int owner_id, int64_t dialog_id, const std::string& sha256, uint64_t size_bytes);

    bool updateProgress(int64_t file_id, uint64_t uploaded_bytes);
    bool markComplete(int64_t file_id);
    bool markFailed(int64_t file_id);

private:
    DB_ConnectionPool& pool_;

    bool setStatus(int64_t file_id, FileStatus status);
};
