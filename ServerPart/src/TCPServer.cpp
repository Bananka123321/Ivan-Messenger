#include "../include/TCPServer.h"

TCPServer::TCPServer(int port) : port(port), serverSocket(-1), sessionManager(), handler(sessionManager) {
    handler.setDisconnectHandler([this](std::shared_ptr<ClientSession> client) {
        clientDisconnect(client);
    }
);
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

        auto client = std::make_shared<ClientSession>(clientSocket);

        std::cout << "CLIENT CONNECTED\n";
        
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(client);
        }

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
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client), clients.end());
    }

    sessionManager.remove(client);

    close(client->getSocket());
    std::cout << "CLIENT WAS DISCONNECTED!\n";
}