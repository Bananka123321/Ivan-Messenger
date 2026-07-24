#include "../include/TcpServer.h"

TcpServer::TcpServer(const int port) : port_(port), serverSocket_(-1), sessionManager_(), handler_(sessionManager_) {
    handler_.setDisconnectHandler([this](const std::shared_ptr<ClientSession> &client) {
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

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (!setupSocket()) return false;

    startClientMonitoring();
    run();

    return true;
}

void TcpServer::stop() {
    if(monitorRunning_.load()) monitorRunning_.store(false);
    if(serverRunning_.load()) serverRunning_.store(false);
    if(monitor_thread_.joinable()) monitor_thread_.join();
    
    if(g_ssl_ctx) {
        SSL_CTX_free(g_ssl_ctx);
        g_ssl_ctx = nullptr;
    }

    EVP_cleanup();
    CONF_modules_unload(1);
    
    if (serverSocket_ != -1) {
        shutdown(serverSocket_, SHUT_RDWR);
        close(serverSocket_);
    }
}

bool TcpServer::setupSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == -1) {
        std::cerr << "Failed to create socket\n";
        return false;
    }

    constexpr int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) { //Forced restart server
        std::cerr << "setsockopt failed\n";
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket_, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == -1) {
        std::cerr << "Bind failed\n";
        return false;
    }

    if (listen(serverSocket_, SOMAXCONN) == -1) {
        std::cerr << "Listen failed\n";
        return false;
    }

    std::cout << "Server listening on port " << port_ << std::endl;
    return true;
}

void TcpServer::run() {
    serverRunning_ = true;
    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr); 
    while (serverRunning_.load()) {
        int clientSocket = accept(serverSocket_, reinterpret_cast<sockaddr *>(&clientAddr), (socklen_t*)&addrLen);
        if (clientSocket == -1) continue;

        SSL* ssl = SSL_new(g_ssl_ctx);
        SSL_set_fd(ssl, clientSocket);

        if (SSL_accept(ssl) <= 0) {
            std::cerr << "TLS Handshake failed\n";
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(clientSocket);
            continue;
        }

        auto client = std::make_shared<ClientSession>(clientSocket, ssl);

        
        std::thread(&TcpServer::handleClient, this, client).detach();
    }
    std::cerr << "!!!SERVER STOPPED!!!\n";
}

void TcpServer::handleClient(const std::shared_ptr<ClientSession>& client) {
    try {
        std::string msg;
        client->setConnected(true);
        while(serverRunning_.load() && client->receive(msg))
            handler_.handleMessage(client, msg);
        clientDisconnect(client);
    } catch(std::exception& e){
        std::cerr << "Client thread ERROR: " << e.what() << '\n';
        clientDisconnect(client);
    }
}

void TcpServer::clientDisconnect(const std::shared_ptr<ClientSession>& client) {
    if(!client->getConnected()) return;

    client->setConnected(false);

    shutdown(client->getSocket(), SHUT_RDWR);

    sessionManager_.remove(client);

    close(client->getSocket());
}

void TcpServer::startClientMonitoring() {
    monitorRunning_ = true;
    monitor_thread_ = std::thread([this](){
        while (monitorRunning_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));

            int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

            auto clients = sessionManager_.getAll();

            for (const auto& client : clients) {
                if(now - client->getLastActivity() > SESSION_TIMEOUT_MS) {
                    clientDisconnect(client);
                }
            }
        }
    });

    monitor_thread_.detach();
}