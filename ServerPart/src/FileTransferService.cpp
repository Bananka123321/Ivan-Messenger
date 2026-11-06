#include "../include/FileTransferService.h"
#include "../include/Logger.h"

#include <algorithm>

#include "Validator.h"
#include "protocol.h"

namespace {

UploadProgress uploadError(std::string error, const int64_t file_id = 0, const uint64_t offset = 0) {
    UploadProgress p;
    p.error = std::move(error);
    p.file_id = file_id;
    p.offset = offset;
    return p;
}

UploadProgress uploadState(const FileRecord& f) {
    UploadProgress p;
    p.ok = true;
    p.file_id = f.info.file_id;
    p.offset = f.uploaded_bytes;
    p.status = f.status;
    p.completed = f.status == FileStatus::Complete;
    return p;
}

}

FileTransferService::FileTransferService(DB_ConnectionPool& pool, FileStorage& storage)
    : files_(pool), dialogs_(pool), storage_(storage) {}

std::mutex& FileTransferService::lockFor(const int64_t file_id) {
    return locks_[static_cast<uint64_t>(file_id) % locks_.size()];
}

bool FileTransferService::isParticipant(const int user_id, const int64_t dialog_id) {
    const auto participants = dialogs_.getDialogParticipants(dialog_id);
    return std::ranges::find(participants, user_id) != participants.end();
}

UploadProgress FileTransferService::initUpload(const UploadRequest& r) {
    if (!Validator::isFileKind(r.kind))
        return uploadError("Unsupported file kind");
    if (r.size_bytes == 0)
        return uploadError("File is empty");
    if (r.size_bytes > protocol::FILE_MAX_SIZE)
        return uploadError("File too large (max 100MB)");
    if (const auto e = Validator::filename(r.filename)) return uploadError(*e);
    if (const auto e = Validator::mimeType(r.mime_type)) return uploadError(*e);
    if (const auto e = Validator::sha256Hex(r.sha256)) return uploadError(*e);

    if (!isParticipant(r.owner_id, r.dialog_id)) {
        LOG_WARNING(FILES, "Попытка загрузить файл в чужой диалог. userId=", r.owner_id, " dialogId=", r.dialog_id);
        return uploadError("You are not a participant of this dialog");
    }

    if (const auto existing = files_.findResumable(r.owner_id, r.dialog_id, r.sha256, r.size_bytes)) {
        LOG_INFO(FILES, "Продолжение загрузки. fileId=", existing->info.file_id, " offset=", existing->uploaded_bytes,
                 " из ", existing->info.size_bytes);
        return uploadState(*existing);
    }

    FileRecord record;
    record.info.filename = r.filename;
    record.info.mime_type = r.mime_type;
    record.info.kind = r.kind;
    record.info.size_bytes = r.size_bytes;
    record.info.sha256 = r.sha256;
    record.owner_id = r.owner_id;
    record.dialog_id = r.dialog_id;
    record.storage_name = FileStorage::generateName();

    const auto id = files_.create(record);
    if (!id)
        return uploadError("Database error");

    LOG_INFO(FILES, "Начата загрузка файла. fileId=", *id, " userId=", r.owner_id, " dialogId=", r.dialog_id,
             " size=", r.size_bytes, " name='", r.filename, "'");

    record.info.file_id = *id;
    return uploadState(record);
}

UploadProgress FileTransferService::uploadStatus(const int user_id, const int64_t file_id) {
    const auto f = files_.get(file_id);
    if (!f || f->owner_id != user_id)
        return uploadError("File not found", file_id);
    return uploadState(*f);
}

UploadProgress FileTransferService::writeChunk(const int user_id, const int64_t file_id, const uint64_t offset, const std::string_view data) {
    std::lock_guard lock(lockFor(file_id));

    const auto f = files_.get(file_id);
    if (!f || f->owner_id != user_id)
        return uploadError("File not found", file_id);

    if (f->status == FileStatus::Complete) {
        auto state = uploadState(*f);
        state.ok = false;
        state.error = "Upload already completed";
        return state;
    }
    if (f->status == FileStatus::Failed)
        return uploadError("Upload failed, start again", file_id);

    // Сервер принимает чанки строго по порядку. Если клиент ошибся с offset (например, не дождался
    // подтверждения перед обрывом связи), ответ вернёт правильный offset, с которого нужно продолжить.
    if (offset != f->uploaded_bytes)
        return uploadError("Offset mismatch", file_id, f->uploaded_bytes);

    if (data.empty())
        return uploadError("Empty chunk", file_id, f->uploaded_bytes);
    if (data.size() > protocol::FILE_CHUNK_SIZE)
        return uploadError("Chunk too large", file_id, f->uploaded_bytes);
    if (offset + data.size() > f->info.size_bytes)
        return uploadError("Chunk exceeds declared file size", file_id, f->uploaded_bytes);

    if (!storage_.write(f->storage_name, offset, data))
        return uploadError("Storage error", file_id, f->uploaded_bytes);

    const uint64_t received = offset + data.size();
    if (!files_.updateProgress(file_id, received))
        return uploadError("Database error", file_id, f->uploaded_bytes);

    if (received < f->info.size_bytes) {
        UploadProgress p;
        p.ok = true;
        p.file_id = file_id;
        p.offset = received;
        return p;
    }

    // Файл получен целиком: сверяем хэш с заявленным клиентом
    const auto actual = storage_.sha256(f->storage_name, f->info.size_bytes);
    if (!actual || *actual != f->info.sha256) {
        LOG_WARNING(FILES, "Хэш не совпал. fileId=", file_id, " ожидался=", f->info.sha256,
                    " получен=", actual.value_or("<нет>"));
        files_.markFailed(file_id);
        storage_.remove(f->storage_name);
        auto p = uploadError("Hash mismatch", file_id, received);
        p.status = FileStatus::Failed;
        return p;
    }

    if (!files_.markComplete(file_id))
        return uploadError("Database error", file_id, received);

    LOG_INFO(FILES, "Файл загружен и проверен. fileId=", file_id, " size=", received, " sha256=", f->info.sha256.substr(0, 12), "...");

    UploadProgress p;
    p.ok = true;
    p.file_id = file_id;
    p.offset = received;
    p.completed = true;
    p.status = FileStatus::Complete;
    return p;
}

DownloadChunk FileTransferService::readChunk(const int user_id, const int64_t file_id, const uint64_t offset) {
    DownloadChunk chunk;
    chunk.file.file_id = file_id;
    chunk.offset = offset;

    // Одинаковый ответ для «нет файла» и «нет доступа», чтобы нельзя было перебирать чужие file_id
    const auto f = files_.get(file_id);
    if (!f || f->status != FileStatus::Complete || !isParticipant(user_id, f->dialog_id)) {
        if (f && f->status == FileStatus::Complete)
            LOG_WARNING(FILES, "Попытка скачать файл из чужого диалога. userId=", user_id, " fileId=", file_id);
        chunk.error = "File not found";
        return chunk;
    }

    chunk.file = f->info;

    if (offset >= f->info.size_bytes) {
        chunk.error = "Offset out of range";
        return chunk;
    }

    auto data = storage_.read(f->storage_name, offset, protocol::FILE_CHUNK_SIZE);
    if (!data || data->empty()) {
        LOG_CRITICAL(FILES, "Файл есть в БД, но не читается с диска. fileId=", file_id, " storage=", f->storage_name);
        chunk.error = "Storage error";
        return chunk;
    }

    chunk.ok = true;
    chunk.data = std::move(*data);
    return chunk;
}

AttachmentCheck FileTransferService::checkAttachment(const int user_id, const int64_t file_id, const int64_t dialog_id, const MessageType type) {
    AttachmentCheck check;

    const auto f = files_.get(file_id);
    if (!f || f->owner_id != user_id || f->dialog_id != dialog_id) {
        check.error = "File not found";
        return check;
    }
    if (f->status != FileStatus::Complete) {
        check.error = "File upload is not completed";
        return check;
    }
    if (f->info.kind != type) {
        check.error = "Message type does not match file kind";
        return check;
    }

    check.ok = true;
    check.file = f->info;
    return check;
}
