#pragma once

#include <thread>
#include <unistd.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "../../common/protocol.h"
#include "Handler.h"
#include "SessionManager.h"

static SSL_CTX* g_ssl_ctx = nullptr;

class TCPServer {
public:
    TCPServer(int port);
    ~TCPServer();

    bool start();

private:
    int port;
    int serverSocket;
    std::vector<std::shared_ptr<ClientSession>> clients;
    std::mutex clientsMutex;

    SessionManager sessionManager;
    Handler handler;

    bool setupSocket();
    void run();
    void clientDisconnect(std::shared_ptr<ClientSession> client);
    friend class MessageDispatcher;
    void handleClient(std::shared_ptr<ClientSession> client);
};