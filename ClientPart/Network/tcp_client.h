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
#include "MessageRouter.h"
#include <iostream>
#include <thread>

class TCPClient
{
public:
    TCPClient(int port, MessageRouter* msgRouter);
    ~TCPClient();

    bool start();

    void handoverSocket();
    std::function<void(const std::string&)> onMessage;

private:
    int port;
    int clientSocket;
    MessageRouter* router;

    bool setupSocket();
    void run();
};
