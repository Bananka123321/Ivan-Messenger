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
        onMessage(j["from"], j["to"], j["text"]);
    };

    handlers["searchUserResponse"] = [this] (const nlohmann::json& j) {
        onUserSearch(j["result"]);
    };

    handlers["error"] = [this] (const nlohmann::json& j) {
        onError(j["message"]);
    };

    handlers["historyResponse"] = [this] (const nlohmann::json& j) {
        onHistory(j["success"], j["peer_id"], j["messages"], j["error"]);
    };

    handlers["getDialogsResponse"] = [this] (const nlohmann::json& j) {
        onDialogs(j["success"], j["dialogs"], j["error"]);
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

void Handler::onUserList(const std::unordered_map<int, std::string>& users) {
    emit S_userList(users);
}

void Handler::onMessage(const int& from, const int& to, const std::string& text) {
    emit S_Message(from, to, text);
}

void Handler::onUserSearch(const std::vector<User>& users) {
    emit S_UserSearch(users);
}

void Handler::onError(const std::string& text) {
    std::cerr << text << '\n';
}

void Handler::onHistory(const bool& success, const int& peer_id, std::vector<Message> history, const std::string& reason) {
    if(success)
        emit S_HistoryLoaded(peer_id, history);
    else
        onError(reason);
}

void Handler::onDialogs(const bool& success, const std::vector<MetaDialog>& dialogs, const std::string& reason) {
    if(success)
        emit S_DialogsLoaded(dialogs);
    else
        onError(reason);
}
