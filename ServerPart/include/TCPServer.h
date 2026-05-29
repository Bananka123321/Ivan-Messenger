#pragma once

#include <thread>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <mutex>
#include <atomic>

#include "../../common/protocol.h"
#include "Handler.h"
#include "ClientSession.h"
#include "SessionManager.h"

static SSL_CTX* g_ssl_ctx = nullptr;
static int64_t HEARTBEAT_INTERVAL_MS = 30000;
static int64_t SESSION_TIMEOUT_MS = 90000;

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();
    void stop();

private:
    int port;
    int serverSocket;

    std::atomic<bool> serverRunning{false};
    std::atomic<bool> monitorRunning{false};

    std::thread monitor_thread;

    SessionManager sessionManager;
    Handler handler;

    friend class MessageDispatcher;

    bool setupSocket();
    void run();
    void clientDisconnect(std::shared_ptr<ClientSession> client);
    void handleClient(std::shared_ptr<ClientSession> client);

    void startClientMonitoring();
};