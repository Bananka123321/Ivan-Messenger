#pragma once
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#pragma comment(lib, "Ws2_32.lib")
#else
// #include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <string>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <atomic>
#include <QObject>

#include "MessageRouter.h"

static SSL_CTX* g_ssl_ctx = nullptr;

class TCPClient{
public:
    TCPClient(int port, MessageRouter* msgRouter);
    ~TCPClient();

    bool start();

    void handoverSocket();
    std::function<void(const std::string&)> onMessage;
private:
    int port_;
    int clientSocket;
    MessageRouter* router_;

    bool setupSocket();
    void run();

    SSL* ssl;
    std::thread work;
    std::thread ping;
    std::atomic<bool> running{false};
};
