#pragma once

#include <string>
#include "protocol.h"
#include "PacketIO.h"

class MessageRouter {
public:
    MessageRouter();

    void loginRequest(const std::string& login, const std::string& password);
    void registerRequest(const std::string& login, const std::string& password);
    void sendMessage(const int& from, const int& to, const std::string& text);

    void setSocket(int socket);

private:
    int clientSocket;
    void sendPacket(const std::string& msg);
};
