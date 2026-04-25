#include "tcp_client.h"

TCPClient::TCPClient(int port_, MessageRouter* msgRouter_) : port(port_), clientSocket(-1), router(msgRouter_) {}

TCPClient::~TCPClient() {
    if (clientSocket != -1) {
#ifdef _WIN32
        closesocket(clientSocket);
#else
        close(clientSocket);
#endif
    }
}

bool TCPClient::start() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    if (!setupSocket()) return false;
    run();

    return true;
}

bool TCPClient::setupSocket() {
    clientSocket = socket(AF_INET, SOCK_STREAM ,IPPROTO_TCP);
    if (clientSocket == -1) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);

    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0) {
        std::cerr << "Incorrect IP ADRESS\n";
        return false;
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Connect failed\n";
        return false;
    }

    handoverSocket();

    std::cout << "You successfully connected to!!\n";
    return true;
}

void TCPClient::run() {
    std::thread([this]() {
        std::string msg;
        while (PacketIO::recvPacket(clientSocket, msg) && onMessage)
            onMessage(msg);
    }).detach();
}

void TCPClient::handoverSocket() {
    router->setSocket(clientSocket);
}
