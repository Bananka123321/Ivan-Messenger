#include "tcp_client.h"
#include <QString>

QString IPADRESS_dev = "127.0.0.1";
QString IPADRESS_dep = "31.192.108.37";

TCPClient::TCPClient(int port, MessageRouter* msgRouter) : port_(port), clientSocket(-1), router_(msgRouter) {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    g_ssl_ctx = SSL_CTX_new(TLS_client_method());
    if(!g_ssl_ctx) {
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
        close(clientSocket);
    }
}

bool TCPClient::start() {
    if(work.joinable()) work.join();
    if(ssl) {
        SSL_free(ssl);
        ssl = nullptr;
    }
    if(clientSocket != -1) {
        close(clientSocket);
        clientSocket = -1;
    }

    if (!setupSocket()) return false;
    work = std::thread([this](){
        runLoop();
    });
    return true;
}

bool TCPClient::setupSocket() {
    clientSocket = socket(AF_INET, SOCK_STREAM ,IPPROTO_TCP);
    if (clientSocket == -1) {
        return false;
    }

    int flag = 1;
    setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);

    if (inet_pton(AF_INET, IPADRESS_dep.toUtf8().constData(), &serverAddr.sin_addr) <= 0) {
        return false;
    }

    if (::connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        return false;
    }

    ssl = SSL_new(g_ssl_ctx);
    SSL_set_fd(ssl, clientSocket);

    if(SSL_connect(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        return false;
    }

    handoverSocket();

    return true;
}

void TCPClient::handoverSocket() {
    router_->setSSL(ssl);
}

void TCPClient::runLoop() {
    bConnected.store(true);

    QMetaObject::invokeMethod(this, [this](){
        emit connected();
    });

    std::string msg;
    while (PacketIO::recvPacket(ssl, msg))
        onMessage(msg);

    QMetaObject::invokeMethod(this, [this](){
        emit connectionLose();
    });
}

void TCPClient::disconnect() {
    if(!bConnected.load()) return;
    if(ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
    if (clientSocket != -1) {
        close(clientSocket);
    }
    if(work.joinable()) work.join();
}
