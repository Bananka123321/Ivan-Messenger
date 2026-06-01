#pragma once

#include <string>
#include <openssl/ssl.h>
#include <QObject>
#include <mutex>

#include "protocol.h"
#include "PacketIO.h"

class MessageRouter : public QObject{
    Q_OBJECT
public:
    MessageRouter();

    void loginRequest(const std::string& login, const std::string& password);
    void registerRequest(const std::string& login, const std::string& password);
    void sendMessage(const int& from, const int& to, const std::string& text);
    void searchUser(const std::string& text);
    void historyRequest(int peer_id, int last_msg_id);
    void getDialogsRequest();
    void ping();
    void resumeConnectionRequest(const std::string& token);

    void setSSL(SSL* ssl_);

private:
    SSL* ssl;
    std::mutex mutex;
    void sendPacket(const std::string& msg);
};
