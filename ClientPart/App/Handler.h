#pragma once
#include <string>
#include <QObject>
#include <iostream>
#include <unordered_map>

#include "nlohmann/json.hpp"
#include "MetaDialog_Client.h"
#include "Message.h"
#include "protocol.h"

class Handler : public QObject{
    Q_OBJECT
public:
    Handler();

    void handleMessage(std::string_view msg);

private:

    std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> handlers_;

    void onLoginResponse(const bool success, const int user_id, const std::string& login, const std::string& connectionToken, const std::string& sessionToken, const std::string& reason);
    void onRegisterResponse(const bool success, const int user_id, const std::string& login, const std::string& connectionToken, const std::string& sessionToken, const std::string& reason);
    void onSendMessage(const Message& msg);
    void onSearchUserResponse(const std::vector<User>& users);
    void onErrorMessage(const std::string& text);
    void onHistoryResponse(const bool success, const int64_t dialog_id, const std::vector<Message>& messages, const std::string& reason);
    void onGetDialogsResponse(const bool success, const std::vector<MetaDialog_Client>& dialogs, const std::string& reason);
    void onResumeConnectionResponse(const bool success);
    void onResumeSessionResponse(const bool success, const std::string& token);
    void onOpenDialogResponse(bool success, int64_t dialog_id, const User& peer, const std::vector<Message>& messages, const std::string& error);

signals:
    void S_loginSuccess(const std::string& login, const int user_id, const std::string& connectionToken, const std::string& sessionToken);
    void S_loginFailed(const std::string& reason);

    void S_registerSuccess(const std::string& login, const int user_id, const std::string& connectionToken, const std::string& sessionToken);
    void S_registerFailed(const std::string& reason);

    void S_Message(const Message& msg);

    void S_UserSearch(const std::vector<User>& users);

    void S_HistoryLoaded(const int64_t dialog_id, const std::vector<Message>& messages);
    void S_DialogsLoaded(const std::vector<MetaDialog_Client>& dialogs);

    void S_ResumeConnectionSuccess();
    void S_ResumeSessionResponse(const bool success, const std::string& token);

    void S_DialogOpened(bool success, int64_t dialog_id, const User& peer, const std::vector<Message>& messages, const std::string& error);
};
