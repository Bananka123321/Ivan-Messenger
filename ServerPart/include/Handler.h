#pragma once
#include <chrono>

#include "DB_UserManager.h"
#include "DB_MessageManager.h"
#include "DB_DialogManager.h"
#include "DB_DevicesSessionManager.h"
#include "DB_TemporaryTokenManager.h"
#include "FileStorage.h"
#include "FileTransferService.h"
#include "SessionManager.h"
#include "MessageDispatcher.h"
#include "Config.h"
#include "Validator.h"

class Handler {
public:
    Handler(SessionManager& sm, DB_ConnectionPool& pool);
    ~Handler() = default;

    void handleMessage(const std::shared_ptr<ClientSession>& client, std::string_view msg);
    void setDisconnectHandler(const std::function<void(std::shared_ptr<ClientSession>)> &cb);
    
private:
    DB_UserManager userManager_;
    DB_MessageManager messageManager_;
    DB_DialogManager dialogManager_;
    DB_DevicesSessionManager devicesSessionManager_;
    DB_TemporaryTokenManager temporaryTokenManager_;
    FileStorage fileStorage_;
    FileTransferService fileTransfer_;
    SessionManager& sessionManager_;
    MessageDispatcher dispatcher_;

    using MessageHandlerFunc = std::function<void(const std::shared_ptr<ClientSession>&, const nlohmann::json&)>;
    std::unordered_map<std::string, MessageHandlerFunc> handlers_;

private:
    void loginRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void registerRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void sendMessage(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void searchUserRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void historyRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void getDialogsRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    static void ping(const std::shared_ptr<ClientSession> &client, const nlohmann::json& j) ;
    void resumeConnectionRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void resumeSessionRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void openDialogRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);

    void fileUploadInitRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void fileUploadStatusRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void fileChunk(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);
    void fileDownloadRequest(const std::shared_ptr<ClientSession>& client, const nlohmann::json& j);

    // Для сообщений с файлом: проверяет вложение и подставляет имя, размер и тип из БД
    std::optional<std::string> attachFile(int userId, Message& msg);
    
    std::string authSuccess(const std::shared_ptr<ClientSession>& client, int id, const std::string& username);

    static std::string generateConnectToken();
};