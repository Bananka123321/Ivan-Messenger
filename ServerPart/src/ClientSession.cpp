#include "../include/ClientSession.h"

ClientSession::ClientSession(int sock) : socket(sock) {}

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
    return PacketIO::sendPacket(socket, message);
}   

bool ClientSession::receive(std::string& message) {
    return PacketIO::recvPacket(socket, message);
}

void ClientSession::setUser(const int& new_id, const std::string& new_username) {
    username = new_username;
    user_id = new_id;
}