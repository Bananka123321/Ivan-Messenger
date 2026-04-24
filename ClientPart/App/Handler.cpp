#include "Handler.h"

Handler::Handler() {
    handlers["loginResponse"] = [this] (const nlohmann::json& j) {
        onLoginResponse(j["success"], j["username"], j["error"], j["user_id"]);
    };

    handlers["registerResponse"] = [this] (const nlohmann::json& j) {
        onRegisterResponse(j["success"], j["username"], j["error"], j["user_id"]);
    };

    handlers["userList"] = [this] (const nlohmann::json& j) {
        onUserList(j["users"]);
    };

    handlers["privateMessage"] = [this] (const nlohmann::json& j) {
        onMessage(j["from"], j["text"]);
    };
}

void Handler::handleMessage(const std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        std::string type = j["type"];

        if (handlers.find(type) != handlers.end())
            handlers[type](j);
        else
            std::cerr << "Unknown message type " << type << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "JSON parse ERROR " << e.what() << std::endl;
    }
}

//=============================================================================================

void Handler::onLoginResponse(const bool& success, const std::string& login, const std::string& reason, const int& user_id) {
    if (success) {
        emit S_loginSuccess(login, user_id);
    } else
        emit S_loginFailed(reason);
}

void Handler::onRegisterResponse(const bool& success, const std::string& login, const std::string& reason, const int& user_id) {
    if (success) {
        emit S_registerSuccess(login, user_id);
    } else
        emit S_registerFailed(reason);
}

void Handler::onUserList(const std::vector<std::string>& users) {
    emit S_userList(users);
}

void Handler::onMessage(const std::string& from, const std::string& text) {
    emit S_Message(from, text);
}
