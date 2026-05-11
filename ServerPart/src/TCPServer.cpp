#include "../include/TCPServer.h"

TCPServer::TCPServer(int port) : port(port), serverSocket(-1), sessionManager(), handler(sessionManager) {
    handler.setDisconnectHandler([this](std::shared_ptr<ClientSession> client) {
        clientDisconnect(client);
    });

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    g_ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!g_ssl_ctx) {
        std::cerr << "Failed to create SSL context\n";
        return;
    }

    if (SSL_CTX_use_certificate_file(g_ssl_ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load certificate\n";
        return;
    }
    
    if (SSL_CTX_use_PrivateKey_file(g_ssl_ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        std::cerr << "Failed to load private key\n";
        return;
    }

    if (!SSL_CTX_check_private_key(g_ssl_ctx)) {
        std::cerr << "Failed to check private key\n";
        return;
    }
}

TCPServer::~TCPServer() {
    if (serverSocket != -1) close(serverSocket);
}

bool TCPServer::start() {
    if (!setupSocket()) return false;
    run();

    return true;
}

bool TCPServer::setupSocket() {
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == -1) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { //Forced sestart server
        std::cerr << "setsockopt failed\n";
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == -1) {
        std::cerr << "Bind failed\n";
        return false;
    }

    if (listen(serverSocket, SOMAXCONN) == -1) { 
        std::cerr << "Listen failed\n";
        return false;
    }

    std::cout << "Server listening on port " << port << std::endl;
    return true;
}

void TCPServer::run() {
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr); 
    while (true) {
        int clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, (socklen_t*)&addrLen);
        if (clientSocket == -1) continue;

        SSL* ssl = SSL_new(g_ssl_ctx);
        SSL_set_fd(ssl, clientSocket);

        if (SSL_accept(ssl) <= 0) {
            std::cerr << "TLS Hanshake failed\n";
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(clientSocket);
            continue;
        }

        auto client = std::make_shared<ClientSession>(clientSocket, ssl);

        std::cout << "CLIENT CONNECTED\n";
        
        std::thread(&TCPServer::handleClient, this, client).detach();
    }
}

void TCPServer::handleClient(std::shared_ptr<ClientSession> client) {
    try {
        std::string msg;
        while(client->receive(msg))
            handler.handleMessage(client, msg);
        clientDisconnect(client);
    } catch(std::exception& e){
        std::cerr << "Client thread ERROR: " << e.what() << '\n';
    }
}

void TCPServer::clientDisconnect(std::shared_ptr<ClientSession> client) {
    sessionManager.remove(client);

    close(client->getSocket());
    std::cout << "CLIENT WAS DISCONNECTED!\n";
}