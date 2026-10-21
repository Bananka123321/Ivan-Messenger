#pragma once

#include <string>
#include <openssl/ssl.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <mutex>

#include "../include/PacketIO_Server.h"

constexpr size_t MAX_OUTGOING_QUEUE_BYTES = 16 * MAX_PACKET_SIZE;

class ClientSession {
public:
    ClientSession(int sock, SSL* ssl);
    ~ClientSession();

    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    [[nodiscard]] int getSocket() const;

    [[nodiscard]] const std::string& getUsername() const;
    [[nodiscard]] int getUserId() const;
    [[nodiscard]] bool getIsAuthenticated() const;
    [[nodiscard]] int64_t getLastActivity() const;
    [[nodiscard]] bool getConnected() const;
    [[nodiscard]] std::string getTempToken() const;

    void setUser(int new_id, const std::string& new_username);
    void setIsAuthenticated(bool value);
    void setLastActivity(int64_t newTimestamp);
    void setConnected(bool newState);
    void setTempToken(const std::string& newToken);

    bool send(const std::string& message);
    bool flush();
    bool receive(std::string& message);

    bool handshake(int timeoutMs);
    void serve(const std::function<void(const std::string&)>& onPacket, const std::atomic<bool>& running);

    bool close();
    [[nodiscard]] bool isClosed() const;
    [[nodiscard]] bool hasPendingOutput();

private:
    enum class ReadResult { Data, WouldBlock, Failed };

    ReadResult readSome();
    bool dispatchPackets(const std::function<void(const std::string&)>& onPacket);
    bool waitSocket(short events, int timeoutMs) const;
    void wake() const;
    void drainWake() const;
    void markBroken(const char* operation, int result);

    int socket_;
    int user_id_;
    std::string username_;
    std::string tempToken_;
    SSL* ssl_;
    std::atomic<bool> isAuthenticated_;
    std::atomic<int64_t> last_activity_time_{0};
    std::atomic<bool> connected_{false};

    int wakeFd_;
    std::atomic<bool> closed_{false};
    bool broken_ = false;

    std::mutex queueMutex_;
    std::deque<std::string> outgoing_;
    size_t queuedBytes_ = 0;

    std::string writeBuffer_;
    size_t writeOffset_ = 0;
    bool readWantsWrite_ = false;

    std::string readBuffer_;
};
