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

    [[nodiscard]] int getSocket() const;
    [[nodiscard]] const std::string& getUsername() const;

    [[nodiscard]] const int& getUserId() const;
    void setUser(const int& new_id, const std::string& new_username);

    [[nodiscard]] bool get_is_authentificate() const;
    void set_is_authentificated(bool value);

    [[nodiscard]] bool send(const std::string& message) const;
    bool receive(std::string& message) const;

    [[nodiscard]] int64_t getLastActivity() const;
    void setLastActivity(const int64_t& newTimestamp);

    [[nodiscard]] bool getConnected() const;
    void setConnected(const bool& newState);

private:
    int socket;
    int user_id{};
    std::string username;
    SSL* ssl;
    bool isAuthentificated;
    std::atomic<int64_t> last_activity_time{0};
    std::atomic<bool> connected{false};
};