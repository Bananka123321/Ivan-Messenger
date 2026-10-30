#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <variant>

#include "Base64.h"
#include "FileInfo.h"
#include "Message.h"
#include "MetaDialog_Client.h"
#include "User.h"


//======================== USER ======================================

inline void to_json(nlohmann::json& j, const User& u) {
    j = {
        {"user_id", u.user_id},
        {"username", u.username}
    };
}
    
inline void from_json(const nlohmann::json& j, User& u) {
    j.at("user_id").get_to(u.user_id);
    j.at("username").get_to(u.username);
}

//======================== Message ===================================

inline void to_json(nlohmann::json& j, const Message& m) {
    j = {
        {"id", m.id},
        {"dialog_id", m.dialog_id},
        {"sender_id", m.sender_id},
        {"type", static_cast<uint8_t>(m.type)},
        {"timestamp", m.created_at_ms},
        {"payload", std::visit([]<typename T0>(const T0& content) {
            nlohmann::json pj;
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, TextContent>) {
                pj["text"] = content.text;
            }
            else if constexpr (std::is_same_v<T, MediaContent>) {
                pj["url"] = content.url;
                pj["filename"] = content.filename;
                pj["size_bytes"] = content.size_bytes;
                pj["caption"] = content.caption;
                pj["file_id"] = content.file_id;
                pj["mime_type"] = content.mime_type;
            }
            else if constexpr (std::is_same_v<T, VoiceContent>) {
                pj["url"] = content.url;
                pj["duration_sec"] = content.duration_sec;
                pj["file_id"] = content.file_id;
                pj["size_bytes"] = content.size_bytes;
            }
            return pj;
        }, m.payload)}
    };
}

inline void from_json(const nlohmann::json& j, Message& m) {
    m.id = j.value("id", int64_t{0});
    m.dialog_id = j.value("dialog_id", 0);
    m.sender_id = j.value("sender_id", 0);
    m.type = static_cast<MessageType>(j.at("type").get<uint8_t>());
    m.created_at_ms = j.value("timestamp", int64_t{0});

    const auto& payload_j = j.at("payload");

    if (m.type == MessageType::Text) {
        m.payload = TextContent{ .text = payload_j.value("text", "") };
    }
    else if (m.type == MessageType::Image || m.type == MessageType::File) {
        m.payload = MediaContent{
            .url = payload_j.value("url", ""),
            .caption = payload_j.value("caption", ""),
            .filename = payload_j.value("filename", ""),
            .size_bytes = payload_j.value("size_bytes", 0ULL),
            .file_id = payload_j.value("file_id", int64_t{0}),
            .mime_type = payload_j.value("mime_type", "")
        };
    }
    else if (m.type == MessageType::Voice) {
        m.payload = VoiceContent{
            .url = payload_j.value("url", ""),
            .duration_sec = payload_j.value("duration_sec", 0U),
            .file_id = payload_j.value("file_id", int64_t{0}),
            .size_bytes = payload_j.value("size_bytes", 0ULL)
        };
    }
    else {
        throw std::runtime_error("Unknown message type in JSON");
    }
}

//======================== MetaDialog ======================================

inline void to_json(nlohmann::json& j, const MetaDialog_Client& md) {
    j = {
        {"dialog_id", md.dialog_id},
        {"peer_id", md.peer_id},
        {"username", md.username},
        {"last_msg_preview", md.last_msg_preview},
        {"last_msg_timestamp", md.last_msg_timestamp}
    };
}

inline void from_json(const nlohmann::json& j, MetaDialog_Client& md) {
    j.at("dialog_id").get_to(md.dialog_id);
    j.at("peer_id").get_to(md.peer_id);
    j.at("username").get_to(md.username);
    j.at("last_msg_preview").get_to(md.last_msg_preview);
    j.at("last_msg_timestamp").get_to(md.last_msg_timestamp);
}

namespace protocol {

constexpr int HISTORY_MAX_LIMIT = 100;

// Размер одного чанка файла до кодирования в base64.
// 256 КБ превращаются примерно в 350 КБ base64 и с запасом влезают в пакет (MAX_PACKET_SIZE = 1 МБ).
constexpr uint32_t FILE_CHUNK_SIZE = 256 * 1024;
constexpr uint64_t FILE_MAX_SIZE = 100ULL * 1024 * 1024;

    
//      CHAT
//=================================================================================================================================================================

// inline std::string broadcastMessage(const std::string& sender, const std::string& text) {
//     nlohmann::json j;
//     j["type"] = "broadcastMessage";
//     j["from"] = sender;
//     j["text"] = text;
//     return j.dump();
// }


//      CLIENT --> SERVER
//=================================================================================================================================================================

inline std::string registerRequest(const std::string& nickname, const std::string& password) {
    nlohmann::json j;
    j["type"] = "registerRequest";
    j["username"] = nickname;
    j["password"] = password;
    return j.dump();
}

inline std::string loginRequest(const std::string& nickname, const std::string& pass) {
    nlohmann::json j;
    j["type"] = "loginRequest";
    j["username"] = nickname;
    j["password"] = pass;
    return j.dump();
}

inline std::string searchUserRequest(const std::string& username) {
    nlohmann::json j;
    j["type"] = "searchUserRequest";
    j["username"] = username;
    return j.dump();
}

inline std::string historyRequest(const int64_t dialog_id, const int64_t last_msg_id, int limit) {
    nlohmann::json j;
    j["type"] = "historyRequest";
    j["dialog_id"] = dialog_id;
    j["last_msg_id"] = last_msg_id;
    j["limit"] = limit;
    return j.dump();
}

inline std::string getDialogsRequest() {
    nlohmann::json j;
    j["type"] = "getDialogsRequest";
    return j.dump();
}

inline std::string ping() {
    nlohmann::json j;
    j["type"] = "ping";
    return j.dump();
}

inline std::string resumeConnectionRequest(const std::string& token) {
    nlohmann::json j;
    j["type"] = "resumeConnectionRequest";
    j["token"] = token;
    return j.dump();
}

inline std::string resumeSessionRequest(const std::string& token) {
    nlohmann::json j;
    j["type"] = "resumeSessionRequest";
    j["token"] = token;
    return j.dump();
}

inline std::string openDialogRequest(int64_t target_user_id) {
    nlohmann::json j;
    j["type"] = "openDialogRequest";
    j["target_user_id"] = target_user_id;
    return j.dump();
}

//      FILES: CLIENT --> SERVER
//
// Загрузка:   fileUploadInitRequest -> fileUploadInitResponse(file_id, offset)
//             fileChunk(offset) -> fileChunkAck(offset) ... пока offset != size_bytes
//             на последнем чанке сервер сверяет SHA-256 и отвечает completed = true
// Докачка:    fileUploadStatusRequest(file_id) -> fileUploadStatusResponse(offset),
//             либо повторный fileUploadInitRequest с тем же хэшем: сервер вернёт прежний file_id и offset
// Отправка:   sendMessage с типом Image/File/Voice и file_id в payload
// Скачивание: fileDownloadRequest(file_id, offset) -> fileDownloadResponse(чанк), клиент сам запрашивает следующий
//=================================================================================================================================================================

inline std::string fileUploadInitRequest(int64_t dialog_id, const std::string& filename, const std::string& mime_type,
                                         MessageType kind, uint64_t size_bytes, const std::string& sha256) {
    nlohmann::json j;
    j["type"] = "fileUploadInitRequest";
    j["dialog_id"] = dialog_id;
    j["filename"] = filename;
    j["mime_type"] = mime_type;
    j["kind"] = static_cast<uint8_t>(kind);
    j["size_bytes"] = size_bytes;
    j["sha256"] = sha256;
    return j.dump();
}

inline std::string fileUploadStatusRequest(int64_t file_id) {
    nlohmann::json j;
    j["type"] = "fileUploadStatusRequest";
    j["file_id"] = file_id;
    return j.dump();
}

inline std::string fileChunk(int64_t file_id, uint64_t offset, std::string_view bytes) {
    nlohmann::json j;
    j["type"] = "fileChunk";
    j["file_id"] = file_id;
    j["offset"] = offset;
    j["data"] = base64::encode(bytes);
    return j.dump();
}

inline std::string fileDownloadRequest(int64_t file_id, uint64_t offset) {
    nlohmann::json j;
    j["type"] = "fileDownloadRequest";
    j["file_id"] = file_id;
    j["offset"] = offset;
    return j.dump();
}

//      SERVER --> CLIENT
//=================================================================================================================================================================

inline std::string sendMessage(const Message& msg) {
    nlohmann::json j;
    j["type"] = "sendMessage";
    j["data"] = msg;
    return j.dump();
}

inline std::string loginResponse(bool success, int user_id, const std::string& login, const std::string& connectToken, const std::string& sessionToken, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "loginResponse";
    j["success"] = success;
    j["user_id"] = user_id;
    j["username"] = login;
    j["connectToken"] = connectToken;
    j["sessionToken"] = sessionToken;
    j["error"] = reason;
    return j.dump();
}

inline std::string registerResponse(bool success, int user_id, const std::string& login, const std::string& connectToken, const std::string& sessionToken, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "registerResponse";
    j["success"] = success;
    j["user_id"] = user_id;
    j["username"] = login;
    j["connectToken"] = connectToken;
    j["sessionToken"] = sessionToken;
    j["error"] = reason;
    return j.dump();
}

inline std::string searchUserResponse(const std::vector<User>& result) {
    nlohmann::json j;
    j["type"] = "searchUserResponse";
    j["result"] = result;
    return j.dump();
}

inline std::string errorMessage(const std::string& reason) {
    nlohmann::json j;
    j["type"] = "error";
    j["message"] = reason;
    return j.dump();
}

inline std::string historyResponse(bool success, const int64_t dialog_id, const std::vector<Message>& messages, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "historyResponse";
    j["success"] = success;
    j["dialog_id"] = dialog_id;
    j["messages"] = messages;
    j["error"] = reason;
    return j.dump();
}

inline std::string getDialogsResponse(bool success, const std::vector<MetaDialog_Client>& dialogs, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "getDialogsResponse";
    j["success"] = success;
    j["dialogs"] = dialogs;
    j["error"] = reason;
    return j.dump();
}

inline std::string resumeConnectionResponse(bool success) {
    nlohmann::json j;
    j["type"] = "resumeConnectionResponse";
    j["success"] = success;
    return j.dump();
}

inline std::string resumeSessionResponse(bool success, const std::string& token = "") {
    nlohmann::json j;
    j["type"] = "resumeSessionResponse";
    j["success"] = success;
    j["token"] = token;
    return j.dump();
}

inline std::string openDialogResponse(bool success, int64_t dialog_id, const User& peer, const std::vector<Message>& messages, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "openDialogResponse";
    j["success"] = success;

    if (success) {
        j["dialog_id"] = dialog_id;
        j["peer"] = peer;
        j["messages"] = messages;
    } else {
        j["error"] = reason;
    }

    return j.dump();
}

//      FILES: SERVER --> CLIENT
//=================================================================================================================================================================

inline std::string fileUploadInitResponse(bool success, int64_t file_id, uint64_t offset, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "fileUploadInitResponse";
    j["success"] = success;
    j["file_id"] = file_id;
    j["offset"] = offset;
    j["chunk_size"] = FILE_CHUNK_SIZE;
    j["error"] = reason;
    return j.dump();
}

inline std::string fileUploadStatusResponse(bool success, int64_t file_id, uint64_t offset, const std::string& status, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "fileUploadStatusResponse";
    j["success"] = success;
    j["file_id"] = file_id;
    j["offset"] = offset;
    j["status"] = status;
    j["error"] = reason;
    return j.dump();
}

// offset — сколько байт сервер уже принял. При success = false клиент продолжает с этого offset.
inline std::string fileChunkAck(bool success, int64_t file_id, uint64_t offset, bool completed, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "fileChunkAck";
    j["success"] = success;
    j["file_id"] = file_id;
    j["offset"] = offset;
    j["completed"] = completed;
    j["error"] = reason;
    return j.dump();
}

inline std::string fileDownloadResponse(bool success, const FileInfo& file, uint64_t offset, std::string_view bytes, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "fileDownloadResponse";
    j["success"] = success;
    j["file_id"] = file.file_id;
    j["offset"] = offset;
    j["data"] = base64::encode(bytes);
    j["filename"] = file.filename;
    j["mime_type"] = file.mime_type;
    j["kind"] = static_cast<uint8_t>(file.kind);
    j["size_bytes"] = file.size_bytes;
    j["sha256"] = file.sha256;
    j["error"] = reason;
    return j.dump();
}

}
