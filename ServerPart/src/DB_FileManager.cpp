#include "../include/DB_FileManager.h"
#include "../include/Logger.h"

#include <pqxx/pqxx>

namespace {

constexpr auto kSelectColumns =
    "SELECT id, owner_id, dialog_id, original_name, mime_type, kind, size_bytes, sha256, "
    "storage_name, uploaded_bytes, status FROM files ";

FileStatus statusFromString(const std::string& s) {
    if (s == "complete") return FileStatus::Complete;
    if (s == "failed") return FileStatus::Failed;
    return FileStatus::Uploading;
}

FileRecord fromRow(const pqxx::row& row) {
    FileRecord f;
    f.info.file_id = row["id"].as<int64_t>();
    f.info.filename = row["original_name"].as<std::string>();
    f.info.mime_type = row["mime_type"].as<std::string>();
    f.info.kind = static_cast<MessageType>(row["kind"].as<int>());
    f.info.size_bytes = row["size_bytes"].as<uint64_t>();
    f.info.sha256 = row["sha256"].as<std::string>();
    f.owner_id = row["owner_id"].as<int>();
    f.dialog_id = row["dialog_id"].as<int64_t>();
    f.storage_name = row["storage_name"].as<std::string>();
    f.uploaded_bytes = row["uploaded_bytes"].as<uint64_t>();
    f.status = statusFromString(row["status"].as<std::string>());
    return f;
}

}

std::string toString(const FileStatus status) {
    switch (status) {
        case FileStatus::Uploading: return "uploading";
        case FileStatus::Complete: return "complete";
        case FileStatus::Failed: return "failed";
    }
    return "uploading";
}

DB_FileManager::DB_FileManager(DB_ConnectionPool& pool) : pool_(pool) {
    LOG_INFO(DB, "Инициализация DB_FileManager");
}

std::optional<int64_t> DB_FileManager::create(const FileRecord& file) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec(
            "INSERT INTO files (owner_id, dialog_id, original_name, mime_type, kind, size_bytes, sha256, storage_name)"
            " VALUES ($1, $2, $3, $4, $5, $6, $7, $8) RETURNING id",
            pqxx::params(file.owner_id, file.dialog_id, file.info.filename, file.info.mime_type,
                         static_cast<int>(file.info.kind), static_cast<int64_t>(file.info.size_bytes),
                         file.info.sha256, file.storage_name));
        txn.commit();

        const auto id = result[0][0].as<int64_t>();
        LOG_DEBUG(DB, "Создана запись файла. fileId=", id, " ownerId=", file.owner_id, " dialogId=", file.dialog_id,
                  " size=", file.info.size_bytes);
        return id;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка создания записи файла. ownerId=", file.owner_id, " ошибка=", e.what());
        return std::nullopt;
    }
}

std::optional<FileRecord> DB_FileManager::get(const int64_t file_id) {
    try {
        auto conn = pool_.acquire();
        pqxx::read_transaction txn(*conn);
        const auto rows = txn.exec(std::string(kSelectColumns) + "WHERE id = $1", pqxx::params(file_id));
        if (rows.empty())
            return std::nullopt;
        return fromRow(rows[0]);
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка чтения записи файла. fileId=", file_id, " ошибка=", e.what());
        return std::nullopt;
    }
}

std::optional<FileRecord> DB_FileManager::findResumable(const int owner_id, const int64_t dialog_id,
                                                        const std::string& sha256, const uint64_t size_bytes) {
    try {
        auto conn = pool_.acquire();
        pqxx::read_transaction txn(*conn);
        const auto rows = txn.exec(
            std::string(kSelectColumns) +
            "WHERE owner_id = $1 AND dialog_id = $2 AND sha256 = $3 AND size_bytes = $4 AND status = 'uploading'"
            " ORDER BY id DESC LIMIT 1",
            pqxx::params(owner_id, dialog_id, sha256, static_cast<int64_t>(size_bytes)));
        if (rows.empty())
            return std::nullopt;
        return fromRow(rows[0]);
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка поиска незавершённой загрузки. ownerId=", owner_id, " ошибка=", e.what());
        return std::nullopt;
    }
}

bool DB_FileManager::updateProgress(const int64_t file_id, const uint64_t uploaded_bytes) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec(
            "UPDATE files SET uploaded_bytes = $2 WHERE id = $1 AND status = 'uploading'",
            pqxx::params(file_id, static_cast<int64_t>(uploaded_bytes)));
        txn.commit();
        return result.affected_rows() == 1;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка обновления прогресса файла. fileId=", file_id, " ошибка=", e.what());
        return false;
    }
}

bool DB_FileManager::markComplete(const int64_t file_id) {
    return setStatus(file_id, FileStatus::Complete);
}

bool DB_FileManager::markFailed(const int64_t file_id) {
    return setStatus(file_id, FileStatus::Failed);
}

bool DB_FileManager::setStatus(const int64_t file_id, const FileStatus status) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec(
            "UPDATE files SET status = $2::text,"
            " completed_at = CASE WHEN $2::text = 'complete' THEN now() ELSE completed_at END"
            " WHERE id = $1",
            pqxx::params(file_id, toString(status)));
        txn.commit();
        return result.affected_rows() == 1;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка смены статуса файла. fileId=", file_id, " ошибка=", e.what());
        return false;
    }
}
