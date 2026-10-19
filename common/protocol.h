#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <variant>

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
            }
            else if constexpr (std::is_same_v<T, VoiceContent>) {
                pj["url"] = content.url;
                pj["duration_sec"] = content.duration_sec;
            }
            return pj;
        }, m.payload)}
    };
}

inline void from_json(const nlohmann::json& j, Message& m) {
    m.id = j.value("id", 0);
    m.dialog_id = j.value("dialog_id", 0);
    m.sender_id = j.value("sender_id", 0);
    m.type = static_cast<MessageType>(j.at("type").get<uint8_t>());
    m.created_at_ms = j.value("timestamp", 0);

    const auto& payload_j = j.at("payload");

    if (m.type == MessageType::Text) {
        m.payload = TextContent{ .text = payload_j.value("text", "") };
    }
    else if (m.type == MessageType::Image || m.type == MessageType::File) {
        m.payload = MediaContent{
            .url = payload_j.value("url", ""),
            .caption = payload_j.value("caption", ""),
            .filename = payload_j.value("filename", ""),
            .size_bytes = payload_j.value("size_bytes", 0ULL)
        };
    }
    else if (m.type == MessageType::Voice) {
        m.payload = VoiceContent{
            .url = payload_j.value("url", ""),
            .duration_sec = payload_j.value("duration_sec", 0U)
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

}
