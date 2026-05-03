#include "../include/Handler.h"

Handler::Handler(SessionManager& sm) : sessionManager(sm), dispatcher(sessionManager), userManager(Config::getDB().getConnectionStr()) {
    handlers["loginRequest"] = [this](std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
        loginRequest(client, j);
    };

    handlers["registerRequest"] = [this](std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
        registerRequest(client, j);
    };

    handlers["privateMessage"] = [this](std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
        privateMessage(client, j);
    };

    handlers["searchUserRequest"] = [this](std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
        searchUserRequest(client, j);
    };
}

Handler::~Handler() {}

void Handler::handleMessage(std::shared_ptr<ClientSession> client, std::string& msg) {
    try {
        auto j = nlohmann::json::parse(msg);
        std::string type = j["type"];
        if (handlers.find(type) != handlers.end()) {
            handlers[type](client, j);
        }
        else
            std::cerr << "Unknown message type " << type << std::endl;    

    } catch (const std::exception& e) {
        std::cerr << "JSON parse ERROR " << e.what() << std::endl;
    }
}



//============================================================================================================================



void Handler::loginRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
    auto res = userManager.loginUser(j["username"], j["password"]);
    std::string response = protocol::loginResponse(res.success, res.user_id, j["username"], res.error);
    dispatcher.sendTo(client, response);

    if (!res.success) return;

    authSuccess(client, res.user_id, j["username"]);
}

void Handler::registerRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
    auto res = userManager.registerUser(j["username"],j["password"]);
    std::string response = protocol::registerResponse(res.success, res.user_id, j["username"], res.error);
    dispatcher.sendTo(client, response);

    if (!res.success) return;

    authSuccess(client, res.user_id, j["username"]);
}

void Handler::authSuccess(std::shared_ptr<ClientSession> client, const int& id, const std::string& username) {
    client->setUser(id, username);
    sessionManager.add(client);

    auto allClients = sessionManager.getAll();

    std::unordered_map<int, std::string> users;

    for (auto& c : allClients)
        users[c->getUserId()] = c->getUsername();

    std::string listMsg = protocol::userList(users);

    dispatcher.broadcast(listMsg);
}

void Handler::privateMessage(std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
    auto receiver = sessionManager.getByUserId(j["to"]);
    if (!receiver) return;

    receiver->send(j.dump());
}

void Handler::setDisconnectHandler(std::function<void(std::shared_ptr<ClientSession>)> cb) {
    dispatcher.onDisconnect = cb;
}

void Handler::searchUserRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j) {
    auto users = userManager.searchUsers(j["username"]);

    std::string response = protocol::searchUserResponse(users);
    dispatcher.sendTo(client, response);
}