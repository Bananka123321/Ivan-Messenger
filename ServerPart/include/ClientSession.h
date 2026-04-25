#pragma once

#include <string>

#include "../../common/PacketIO.h"

class ClientSession {
public:
    ClientSession(int sock);

    const int getSocket() const;
    const std::string& getUsername() const;
    const int& getUserId() const;
    void setUser(const int& new_id, const std::string& new_username);

    bool send(const std::string& message);
    bool receive(std::string& message);

private:
    int socket;
    int user_id;
    std::string username;
};