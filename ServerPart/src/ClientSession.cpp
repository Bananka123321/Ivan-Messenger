#include "../include/ClientSession.h"

ClientSession::ClientSession(const int sock, SSL* ssl_) : socket_(sock), user_id_(0), ssl_(ssl_), isAuthenticated_(false),
    last_activity_time_(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).
        count()) {
}

ClientSession::~ClientSession() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
}

int ClientSession::getSocket() const {
    return socket_;
}

const std::string& ClientSession::getUsername() const {
    return username_;
}

const int& ClientSession::getUserId() const {
    return user_id_;
}

bool ClientSession::send(const std::string& message) const {
    return PacketIO::sendPacket(ssl_, message);
}   

bool ClientSession::receive(std::string& message) const {
    return PacketIO::recvPacket(ssl_, message);
}

void ClientSession::setUser(const int& new_id, const std::string& new_username) {
    username_ = new_username;
    user_id_ = new_id;
}

bool ClientSession::get_is_authenticated() const {
    return isAuthenticated_;
}

void ClientSession::set_is_authenticated(const bool value) {
    isAuthenticated_ = value;
}

int64_t ClientSession::getLastActivity() const {
    return last_activity_time_.load(std::memory_order_relaxed);
}

void ClientSession::setLastActivity(const int64_t& newTimestamp) {
    last_activity_time_.store(newTimestamp);
}

bool ClientSession::getConnected() const {
    return connected_.load(std::memory_order_relaxed);
}

void ClientSession::setConnected(const bool& newState) {
    connected_.store(newState);
}