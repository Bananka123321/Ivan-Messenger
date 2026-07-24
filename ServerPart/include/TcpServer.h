#pragma once

#include <thread>
#include <unistd.h>
#include <atomic>

#include "Handler.h"
#include "ClientSession.h"
#include "SessionManager.h"

static SSL_CTX* g_ssl_ctx = nullptr;
static int64_t HEARTBEAT_INTERVAL_MS = 30000;
static int64_t SESSION_TIMEOUT_MS = 90000;

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    bool start();
    void stop();

private:
    int port_;
    int serverSocket_;

    std::atomic<bool> serverRunning_{false};
    std::atomic<bool> monitorRunning_{false};

    std::thread monitor_thread_;

    SessionManager sessionManager_;
    Handler handler_;

    friend class MessageDispatcher_;

private:
    bool setupSocket();
    void run();
    void clientDisconnect(const std::shared_ptr<ClientSession>& client);
    void handleClient(const std::shared_ptr<ClientSession>& client);

    void startClientMonitoring();
};