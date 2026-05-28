#include "../include/ClientSession.h"

ClientSession::ClientSession(int sock, SSL* ssl_) : socket(sock), ssl(ssl_), isAuthentificated(false) {}

ClientSession::~ClientSession() {
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        ssl = nullptr;
    }
}

const int ClientSession::getSocket() const {
    return socket;
}

const std::string& ClientSession::getUsername() const {
    return username;
}

const int& ClientSession::getUserId() const {
    return user_id;
}

bool ClientSession::send(const std::string& message) {
    
    return PacketIO::sendPacket(ssl, message);
}   

bool ClientSession::receive(std::string& message) {
    return PacketIO::recvPacket(ssl, message);
}

void ClientSession::setUser(const int& new_id, const std::string& new_username) {
    username = new_username;
    user_id = new_id;
}

bool ClientSession::getIsAuthentificated() const {
    return isAuthentificated;
}

void ClientSession::setIsAuthentificated(bool value) {
    isAuthentificated = value;
}