#pragma once
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

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
    explicit Handler(SessionManager& sm);
    ~Handler();

    void handleMessage(const std::shared_ptr<ClientSession>& client, std::string& msg);
    void setDisconnectHandler(const std::function<void(std::shared_ptr<ClientSession>)> &cb);
    
private:
    UserManager userManager;
    messageManager msgManager;
    SessionManager& sessionManager;
    MessageDispatcher dispatcher;
    DB_DialogManager diaManager;

    std::unordered_map<std::string, std::function<void(std::shared_ptr<ClientSession>, const nlohmann::json&)>> handlers;

    void loginRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void registerRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void privateMessage(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void searchUserRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void historyRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void getDialogsRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void ping(const std::shared_ptr<ClientSession> &client, const nlohmann::json& j) const;
    void resumeConnectionRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    
    void authSuccess(const std::shared_ptr<ClientSession>& client, const int& id, const std::string& username);

    static std::string generateToken();
};