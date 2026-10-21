#include "../include/TcpServer.h"
#include "../include/Logger.h"
#include <openssl/err.h>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>

TcpServer::TcpServer(const int port)
    : port_(port), serverSocket_(-1), ssl_ctx_(nullptr),
      dbPool_(Config::getDB().getConnectionStr(), Config::getDB().pool_size), sessionManager_(),
      handler_(sessionManager_, dbPool_), temporaryTokenManager_(dbPool_)
{
    LOG_INFO(MAIN, "Инициализация TcpServer на порту ", port_);

    handler_.setDisconnectHandler([this](const std::shared_ptr<ClientSession> &client) {
        clientDisconnect(client);
    });

    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();

    ssl_ctx_ = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx_) {
        LOG_CRITICAL(MAIN, "Критическая ошибка: не удалось создать SSL контекст");
        ERR_print_errors_fp(stderr);
        return;
    }

    if (SSL_CTX_use_certificate_file(ssl_ctx_, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        LOG_CRITICAL(MAIN, "Критическая ошибка: не удалось загрузить сертификат server.crt");
        ERR_print_errors_fp(stderr);
        return;
    }

    if (SSL_CTX_use_PrivateKey_file(ssl_ctx_, "server.key", SSL_FILETYPE_PEM) <= 0) {
        LOG_CRITICAL(MAIN, "Критическая ошибка: не удалось загрузить приватный ключ server.key");
        ERR_print_errors_fp(stderr);
        return;
    }

    if (!SSL_CTX_check_private_key(ssl_ctx_)) {
        LOG_CRITICAL(MAIN, "Критическая ошибка: несоответствие приватного ключа и сертификата");
        ERR_print_errors_fp(stderr);
        return;
    }

    LOG_INFO(MAIN, "SSL контекст успешно инициализирован");
}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    LOG_INFO(MAIN, "Запуск сервера...");
    if (!setupSocket()) {
        LOG_CRITICAL(MAIN, "Не удалось запустить сервер: ошибка настройки сокета");
        return false;
    }

    startClientMonitoring();
    run();

    return true;
}

void TcpServer::stop() {
    LOG_INFO(MAIN, "Остановка сервера...");

    if(monitorRunning_.load()) monitorRunning_.store(false);
    if(serverRunning_.load()) serverRunning_.store(false);

    if (serverSocket_ != -1) {
        shutdown(serverSocket_, SHUT_RDWR);
        close(serverSocket_);
        serverSocket_ = -1;
    }

    if(monitor_thread_.joinable()) {
        monitor_thread_.join();
    }

    if(ssl_ctx_) {
        SSL_CTX_free(ssl_ctx_);
        ssl_ctx_ = nullptr;
    }

    EVP_cleanup();
    CONF_modules_unload(1);

    LOG_INFO(MAIN, "Сервер успешно остановлен");
}

bool TcpServer::setupSocket() {
    serverSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket_ == -1) {
        LOG_CRITICAL(MAIN, "Не удалось создать сокет: ", strerror(errno));
        return false;
    }

    constexpr int opt = 1;
    if (setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARNING(MAIN, "Не удалось установить SO_REUSEADDR: ", strerror(errno));
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port_);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket_, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == -1) {
        LOG_CRITICAL(MAIN, "Не удалось выполнить bind на порт ", port_, ": ", strerror(errno));
        return false;
    }

    if (listen(serverSocket_, SOMAXCONN) == -1) {
        LOG_CRITICAL(MAIN, "Не удалось выполнить listen: ", strerror(errno));
        return false;
    }

    LOG_INFO(MAIN, "Сервер успешно запущен и слушает порт ", port_);
    return true;
}

void TcpServer::run() {
    serverRunning_ = true;
    LOG_INFO(MAIN, "Цикл принятия подключений (accept loop) запущен");

    sockaddr_in clientAddr{};
    socklen_t addrLen = sizeof(clientAddr);

    while (serverRunning_.load()) {
        int clientSocket = accept(serverSocket_, reinterpret_cast<sockaddr *>(&clientAddr), &addrLen);

        if (clientSocket == -1) {
            if (serverRunning_.load()) {
                LOG_WARNING(MAIN, "Ошибка accept: ", strerror(errno));
            }
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, ipStr, INET_ADDRSTRLEN);

        LOG_INFO(NETWORK, "Новое подключение. socket_fd=", clientSocket, " ip=", ipStr);

        if (fcntl(clientSocket, F_SETFL, fcntl(clientSocket, F_GETFL, 0) | O_NONBLOCK) == -1) {
            LOG_WARNING(NETWORK, "Не удалось перевести сокет в неблокирующий режим. socket_fd=", clientSocket, " ошибка=", strerror(errno));
            close(clientSocket);
            continue;
        }

        SSL* ssl = SSL_new(ssl_ctx_);
        SSL_set_fd(ssl, clientSocket);

        auto client = std::make_shared<ClientSession>(clientSocket, ssl);

        std::thread(&TcpServer::handleClient, this, client).detach();
    }

    LOG_INFO(MAIN, "Цикл принятия подключений остановлен");
}

void TcpServer::handleClient(const std::shared_ptr<ClientSession>& client) {
    LOG_DEBUG(NETWORK, "Поток обработки клиента запущен. socket_fd=", client->getSocket());

    if (!client->handshake(HANDSHAKE_TIMEOUT_MS)) {
        LOG_WARNING(NETWORK, "Ошибка TLS Handshake для socket_fd=", client->getSocket());
        return;
    }

    LOG_INFO(NETWORK, "TLS Handshake успешен. socket_fd=", client->getSocket());

    try {
        client->setConnected(true);

        client->serve([this, &client](const std::string& msg) {
            handler_.handleMessage(client, msg);
        }, serverRunning_);

        LOG_INFO(NETWORK, "Клиент завершил соединение (нормальный выход из цикла receive). socket_fd=", client->getSocket(), " userId=", client->getUserId());
        clientDisconnect(client);

    } catch(const std::exception& e) {
        LOG_WARNING(NETWORK, "Необработанное исключение в потоке клиента. socket_fd=", client->getSocket(), " userId=", client->getUserId(), " ошибка=", e.what());
        clientDisconnect(client);
    }
}

void TcpServer::clientDisconnect(const std::shared_ptr<ClientSession>& client) {
    if(!client || !client->close()) {
        return;
    }

    LOG_INFO(NETWORK, "Инициализация корректного отключения клиента. socket_fd=", client->getSocket(), " userId=", client->getUserId());

    client->setConnected(false);

    sessionManager_.remove(client);
    temporaryTokenManager_.deleteSession(client->getTempToken());
}

void TcpServer::startClientMonitoring() {
    monitorRunning_ = true;
    monitor_thread_ = std::thread([this](){
        LOG_INFO(MAIN, "Поток мониторинга сессий запущен. Интервал проверки=", HEARTBEAT_INTERVAL_MS, "мс, Таймаут=", SESSION_TIMEOUT_MS, "мс");

        while (monitorRunning_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));

            const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            for (auto clients = sessionManager_.getAll(); const auto& client : clients) {
                if(now - client->getLastActivity() > SESSION_TIMEOUT_MS) {
                    LOG_WARNING(NETWORK, "Таймаут сессии клиента (бездействие). socket_fd=", client->getSocket(), " userId=", client->getUserId());
                    clientDisconnect(client);
                }
            }
        }

        LOG_INFO(MAIN, "Поток мониторинга сессий остановлен");
    });

    monitor_thread_.detach();
}