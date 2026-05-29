#pragma once

#include <string>
#include <openssl/ssl.h>
#include <atomic>
#include <chrono>

#include "../../common/PacketIO.h"

class ClientSession {
public:
    ClientSession(int sock, SSL* ssl_);
    ~ClientSession();

    const int getSocket() const;
    const std::string& getUsername() const;

    const int& getUserId() const;
    void setUser(const int& new_id, const std::string& new_username);

    bool getIsAuthentificated() const;
    void setIsAuthentificated(bool value);

    bool send(const std::string& message);
    bool receive(std::string& message);

    int64_t getLastActivity() const;
    void setLastActivity(const int64_t& newTimestamp);

    bool getConnected() const;
    void setConnected(const bool& newState);

private:
    int socket;
    int user_id;
    std::string username;
    SSL* ssl;
    bool isAuthentificated;
    std::atomic<int64_t> last_activity_time{0};
    std::atomic<bool> connected{false};
};