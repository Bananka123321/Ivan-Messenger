#include "MessageRouter.h"

MessageRouter::MessageRouter() {}
\
void MessageRouter::setSSL(SSL* ssl_) {
    std::lock_guard<std::mutex> lock(mutex);
    ssl = ssl_;
}

void MessageRouter::setReconnecting(bool value) {
    isReconnecting.store(value);
}

void MessageRouter::loginRequest(const std::string& login, const std::string& password) {
    std::string request = protocol::loginRequest(login, password);
    sendPacket(request);
}

void MessageRouter::registerRequest(const std::string& login, const std::string& password) {
    std::string request = protocol::registerRequest(login, password);
    sendPacket(request);
}

void MessageRouter::sendMessage(const int& from, const int& to, const std::string& text) {
    std::string request = protocol::privateMessage(from, to, text);
    sendPacket(request);
}

void MessageRouter::searchUser(const std::string& text) {
    std::string request = protocol::searchUserRequest(text);
    sendPacket(request);
}

void MessageRouter::historyRequest(int peer_id, int last_msg_id) {
    std::string request = protocol::historyRequest(peer_id, last_msg_id, 200);
    sendPacket(request);
}

void MessageRouter::getDialogsRequest() {
    std::string request = protocol::getDialogsRequest();
    sendPacket(request);
}

void MessageRouter::ping() {
    std::string request = protocol::ping();
    sendPacket(request);
}

void MessageRouter::resumeConnectionRequest(const std::string& token) {
    std::string request = protocol::resumeConnectionRequest(token);
    sendPacket(request, true);
}

void MessageRouter::sendPacket(const std::string& msg, bool force) {
    if(!force && isReconnecting.load()) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex);
    if(!force && isReconnecting.load()) {
        return;
    }

    if(!ssl) {
        return;
    }
    PacketIO::sendPacket(ssl, msg);
}
