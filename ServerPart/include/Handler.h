#pragma once
#include <chrono>

#include "../../common/protocol.h"
#include "DB_UserManager.h"
#include "DB_MessageManager.h"
#include "DB_DialogManager.h"
#include "SessionManager.h"
#include "MessageDispatcher.h"
#include "Config.h"
#include "Validator.h"
#include "../common/MetaDialog.h"

class Handler {
public:
    Handler(SessionManager& sm);
    ~Handler();

    void handleMessage(std::shared_ptr<ClientSession> client, std::string& msg);
    void setDisconnectHandler(std::function<void(std::shared_ptr<ClientSession>)> cb);
    
private:
    UserManager userManager;
    messageManager msgManager;
    SessionManager& sessionManager;
    MessageDispatcher dispatcher;
    DB_DialogManager diaManager;

    std::unordered_map<std::string, std::function<void(std::shared_ptr<ClientSession>, const nlohmann::json&)>> handlers;

    void loginRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    void registerRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    void privateMessage(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    void searchUserRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    void historyRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    void getDialogsRequest(std::shared_ptr<ClientSession> client, const nlohmann::json& j);
    
    void authSuccess(std::shared_ptr<ClientSession> client, const int& id, const std::string& username);
};