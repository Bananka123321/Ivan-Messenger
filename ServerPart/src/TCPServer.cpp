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
    stop();
}

bool TCPServer::start() {
    if (!setupSocket()) return false;

    startClientMonitoring();
    run();

    return true;
}

void TCPServer::stop() {
    if(monitorRunning.load()) monitorRunning.store(false);
    if(serverRunning.load()) serverRunning.store(false);
    if(monitor_thread.joinable()) monitor_thread.join();
    
    if(g_ssl_ctx) {
        SSL_CTX_free(g_ssl_ctx);
        g_ssl_ctx = nullptr;
    }

    EVP_cleanup();
    CONF_modules_unload(1);
    
    if (serverSocket != -1) {
        shutdown(serverSocket, SHUT_RDWR);
        close(serverSocket);
    }
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
    serverRunning = true;
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr); 
    while (serverRunning.load()) {
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

        
        std::thread(&TCPServer::handleClient, this, client).detach();
    }
    std::cerr << "!!!SERVER STOPED!!!\n";
}

void TCPServer::handleClient(std::shared_ptr<ClientSession> client) {
    try {
        std::string msg;
        client->setConnected(true);
        while(serverRunning.load() && client->receive(msg))
            handler.handleMessage(client, msg);
        clientDisconnect(client);
    } catch(std::exception& e){
        std::cerr << "Client thread ERROR: " << e.what() << '\n';
        clientDisconnect(client);
    }
}

void TCPServer::clientDisconnect(std::shared_ptr<ClientSession> client) {
    if(!client->getConnected()) return;

    client->setConnected(false);

    shutdown(client->getSocket(), SHUT_RDWR);

    sessionManager.remove(client);

    close(client->getSocket());
}

void TCPServer::startClientMonitoring() {
    monitorRunning = true;
    monitor_thread = std::thread([this](){
        while (monitorRunning.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));

            int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            auto clients = sessionManager.getAll();

            for (const auto& client : clients) {
                if(now - client->getLastActivity() > SESSION_TIMEOUT_MS) {
                    clientDisconnect(client);
                }
            }
        }
    });

    monitor_thread.detach();
}