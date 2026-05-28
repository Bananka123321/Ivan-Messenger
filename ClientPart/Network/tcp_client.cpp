#include "tcp_client.h"
#include <QString>

QString IPADRESS_dev = "127.0.0.1";
QString IPADRESS_dep = "31.192.108.37";

TCPClient::TCPClient(int port_, MessageRouter* msgRouter_) : port(port_), clientSocket(-1), router(msgRouter_) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    g_ssl_ctx = SSL_CTX_new(TLS_client_method());
    if(!g_ssl_ctx) {
        std::cerr << "Failed to create SSL context\n";
        return;
    }

    SSL_CTX_set_verify(g_ssl_ctx, SSL_VERIFY_NONE, nullptr);
}

TCPClient::~TCPClient() {
    if(ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }

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
    work = std::thread([this]() {
        std::string msg;
        while (PacketIO::recvPacket(ssl, msg) && onMessage)
            onMessage(msg);
    });

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

    if (inet_pton(AF_INET, IPADRESS_dep.toUtf8().constData(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Incorrect IP ADRESS\n";
        return false;
    }

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Connect failed\n";
        return false;
    }

    ssl = SSL_new(g_ssl_ctx);
    SSL_set_fd(ssl, clientSocket);

    if(SSL_connect(ssl) <= 0) {
        std::cerr << "TLS connect failed\n";
        ERR_print_errors_fp(stderr);
        return false;
    }

    handoverSocket();

    return true;
}

void TCPClient::handoverSocket() {
    router->setSSL(ssl);
}
