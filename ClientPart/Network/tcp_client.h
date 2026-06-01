#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET Socket;
#pragma comment(lib, "Ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <string>
#include <iostream>
#include <thread>
#include <atomic>
#include <QObject>

#include "MessageRouter.h"

static SSL_CTX* g_ssl_ctx = nullptr;

class TCPClient : public QObject{
    Q_OBJECT
public:
    TCPClient(int port, MessageRouter* msgRouter);
    ~TCPClient();

    bool start();
    void disconnect();
    bool isConnected() const;

    void handoverSocket();
    std::function<void(const std::string&)> onMessage;

signals:
    void connectionLose();
    void connected();

private:
    int port_;
    int clientSocket;
    MessageRouter* router_;

    bool setupSocket();
    void run();

    SSL* ssl;
    std::thread work;

    std::atomic<bool> bConnected{false};

    void runLoop();
};
