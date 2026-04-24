#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace protocol {

//      CHAT
//=================================================================================================================================================================

inline std::string broadcastMessage(const std::string& sender, const std::string& text) {
    nlohmann::json j;
    j["type"] = "broadcastMessage";
    j["from"] = sender;
    j["text"] = text;
    return j.dump();
}

inline std::string privateMessage(const std::string& sender, const std::string& user, const std::string& text) {
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

inline std::string checkUsernameRequest(const std::string& username) {
    nlohmann::json j;
    j["type"] = "checkUsernameRequest";
    j["username"] = username;
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

inline std::string userList(const std::vector<std::string>& users) {
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

}