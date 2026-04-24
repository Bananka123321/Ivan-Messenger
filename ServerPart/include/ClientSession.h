#pragma once

#include <string>

#include "../../common/PacketIO.h"

class ClientSession {
public:
    ClientSession(int sock);

    const int getSocket() const;
    const std::string& getUsername() const;
    void setUsername(const std::string& new_username);

    bool send(const std::string& message);
    bool receive(std::string& message);

private:
    int socket;
    std::string username;
};