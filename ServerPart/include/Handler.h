#pragma once
#include <chrono>

#include "DB_UserManager.h"
#include "DB_MessageManager.h"
#include "DB_DialogManager.h"
#include "SessionManager.h"
#include "MessageDispatcher_.h"
#include "Config.h"
#include "Validator.h"

class Handler {
public:
    explicit Handler(SessionManager& sm);
    ~Handler() = default;

    void handleMessage(const std::shared_ptr<ClientSession>& client, std::string_view msg);
    void setDisconnectHandler(const std::function<void(std::shared_ptr<ClientSession>)> &cb);
    
private:
    DB_UserManager userManager_;
    DB_MessageManager messageManager_;
    DB_DialogManager dialogManager_;
    SessionManager& sessionManager_;
    MessageDispatcher_ dispatcher_;

    using MessageHandlerFunc = std::function<void(const std::shared_ptr<ClientSession>&, const nlohmann::json&)>;
    std::unordered_map<std::string, MessageHandlerFunc> handlers_;

private:
    void loginRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void registerRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void privateMessage(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void searchUserRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void historyRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void getDialogsRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    static void ping(const std::shared_ptr<ClientSession> &client, const nlohmann::json& j) ;
    void resumeConnectionRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    
    void authSuccess(const std::shared_ptr<ClientSession>& client, int id, const std::string& username);

    static std::string generateToken();
};