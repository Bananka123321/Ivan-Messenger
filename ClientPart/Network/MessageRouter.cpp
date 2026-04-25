#include "MessageRouter.h"

MessageRouter::MessageRouter() {}
\
void MessageRouter::setSocket(int socket) {
    clientSocket = socket;
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

void MessageRouter::sendPacket(const std::string& msg) {
    PacketIO::sendPacket(clientSocket, msg);
}
