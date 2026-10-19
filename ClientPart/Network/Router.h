#pragma once

#include <string>
#include <QSslSocket>
#include <QObject>
#include <mutex>
#include <atomic>

#include "protocol.h"
#include "PacketIO_Client.h"

class Router : public QObject{
    Q_OBJECT
public:
    Router();

    void loginRequest(const std::string& login, const std::string& password);
    void registerRequest(const std::string& login, const std::string& password);
    void sendMessage(const Message& msg);
    void searchUser(const std::string& text);
    void historyRequest(const int64_t dialog_id, const int64_t last_msg_id);
    void getDialogsRequest();
    void ping();
    void resumeConnectionRequest(const std::string& token);
    void resumeSessionRequest(const std::string& token);
    void openDialogRequest(int64_t target_user_id);

    void setSocket(QSslSocket* socket);
    void setReconnecting(bool value);

private:
    QSslSocket* socket_ = nullptr;
    std::mutex mutex_;
    std::atomic<bool> isReconnecting_{false};

private:
    void sendPacket(const std::string& msg, bool force = false);
};
