#include "MessageRouter.h"

MessageRouter::MessageRouter() {}
\
void MessageRouter::setSSL(SSL* ssl_) {
    ssl = ssl_;
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

void MessageRouter::sendPacket(const std::string& msg) {
    PacketIO::sendPacket(ssl, msg);
}
