#include "../include/ClientSession.h"

ClientSession::ClientSession(int sock) : socket(sock) {}

const int ClientSession::getSocket() const {
    return socket;
}

const std::string& ClientSession::getUsername() const {
    return username;
}

bool ClientSession::send(const std::string& message) {
    return PacketIO::sendPacket(socket, message);
}   

bool ClientSession::receive(std::string& message) {
    return PacketIO::recvPacket(socket, message);
}

void ClientSession::setUsername(const std::string& new_username) {
    username = new_username;
}