#pragma once
#include <string>
#include "nlohmann/json.hpp"
#include <QObject>
#include <iostream>
#include <unordered_map>

class Handler : public QObject{
    Q_OBJECT
public:
    Handler();

    void handleMessage(const std::string& msg);

private:

    std::unordered_map<std::string, std::function<void(const nlohmann::json&)>> handlers;

    void onLoginResponse(const bool& success, const std::string& login, const std::string& reason, const int& user_id);
    void onRegisterResponse(const bool& success, const std::string& login, const std::string& reason, const int& user_id);
    void onUserList(const std::unordered_map<int, std::string>& users);
    void onMessage(const int& from, const std::string& text);

signals:
    void S_loginSuccess(const std::string& login, const int& user_id);
    void S_loginFailed(const std::string& reason);

    void S_registerSuccess(const std::string& login, const int& user_id);
    void S_registerFailed(const std::string& reason);

    void S_Message(const int& sender, const std::string& text);
    void S_userList(const std::unordered_map<int, std::string>& users);

};
