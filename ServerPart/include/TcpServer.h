#pragma once

#include <thread>
#include <unistd.h>
#include <atomic>

#include "Handler.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include "DB_TemporaryTokenManager.h"


class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    bool start();
    void stop();

private:
    int port_;
    int serverSocket_;
    SSL_CTX* ssl_ctx_;

    std::atomic<bool> serverRunning_{false};
    std::atomic<bool> monitorRunning_{false};

    std::thread monitor_thread_;

    DB_ConnectionPool dbPool_;
    SessionManager sessionManager_;
    Handler handler_;
    DB_TemporaryTokenManager temporaryTokenManager_;

    static constexpr int64_t HEARTBEAT_INTERVAL_MS = 30000;
    static constexpr int64_t SESSION_TIMEOUT_MS = 90000;
    static constexpr int HANDSHAKE_TIMEOUT_MS = 10000;

    friend class MessageDispatcher;

private:
    bool setupSocket();
    void run();
    void clientDisconnect(const std::shared_ptr<ClientSession>& client);
    void handleClient(const std::shared_ptr<ClientSession>& client);

    void startClientMonitoring();
};