#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include "Message.h"
#include "MetaDialog.h"

struct User {
        int user_id;
        std::string username;
    };

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
        {"message_id", m.msgId},
        {"receiver_id", m.receiverId},
        {"sender_id", m.senderId},
        {"text", m.text},
        {"timestamp", m.timestamp}
    };
}

inline void from_json(const nlohmann::json& j, Message& m) {
    j.at("message_id").get_to(m.msgId);
    j.at("receiver_id").get_to(m.receiverId);
    j.at("sender_id").get_to(m.senderId);
    j.at("text").get_to(m.text);
    j.at("timestamp").get_to(m.timestamp);
}

//======================== MetaDialog ======================================

inline void to_json(nlohmann::json& j, const MetaDialog& md) {
    j = {
        {"peer_id", md.peer_id},
        {"username", md.username}
    };
}

inline void from_json(const nlohmann::json& j, MetaDialog& md) {
    j.at("peer_id").get_to(md.peer_id);
    j.at("username").get_to(md.username);
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

inline std::string privateMessage(const int& sender, const int& user, const std::string& text) {
    nlohmann::json j;
    j["type"] = "privateMessage";
    j["text"] = text;
    j["from"] = sender;
    j["to"] = user;
    return j.dump();
}

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

inline std::string historyRequest(const int& receiver, const int& last_msg_id, const int& limit) {
    nlohmann::json j;
    j["type"] = "historyRequest";
    j["peer_id"] = receiver;
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

//      SERVER --> CLIENT
//=================================================================================================================================================================

inline std::string loginResponse(const bool& success, const int& user_id, const std::string& login, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "loginResponse";
    j["success"] = success;
    j["user_id"] = user_id;
    j["username"] = login;
    j["error"] = reason;
    return j.dump();
}

inline std::string registerResponse(const bool& success, const int& user_id, const std::string& login, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "registerResponse";
    j["success"] = success;
    j["user_id"] = user_id;
    j["username"] = login;
    j["error"] = reason;
    return j.dump();
}

inline std::string userList(const std::unordered_map<int, std::string>& users) {
    nlohmann::json j;
    j["type"] = "userList";
    j["users"] = users;
    return j.dump();
}

inline std::string usernameAvailability(bool available) {
    nlohmann::json j;
    j["type"] = "usernameAvailability";
    j["available"] = available;
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

inline std::string historyResponse(const bool& success, const int& peer_id, const std::vector<Message>& messages, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "historyResponse";
    j["success"] = success;
    j["peer_id"] = peer_id;
    j["messages"] = messages;
    j["error"] = reason;
    return j.dump();
}

inline std::string getDialogsResponse(const bool& success, const std::vector<MetaDialog>& dialogs, const std::string& reason = "") {
    nlohmann::json j;
    j["type"] = "getDialogsResponse";
    j["success"] = success;
    j["dialogs"] = dialogs;
    j["error"] = reason;
    return j.dump();
}

}
