#pragma once

#include "../../common/protocol.h"
#include "UserManager.h"
#include "SessionManager.h"
#include "MessageDispatcher.h"

class Handler {
public:
    Handler(SessionManager& sm);
    ~Handler();

    void handleMessage(std::shared_ptr<ClientSession> client, std::string& msg);
    void setDisconnectHandler(std::function<void(std::shared_ptr<ClientSession>)> cb);
    
private:
    UserManager userManager;
    SessionManager& sessionManager;
    MessageDispatcher dispatcher;

    std::unordered_map<std::string, std::function<void(std::shared_ptr<ClientSession>, const nlohmann::json&)>> handlers;

    void loginRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    void registerRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    void privateMessage(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    
    void authSuccess(std::shared_ptr<ClientSession> client, const std::string& username);
};