#include "PacketIO_Server.h"
#include <iostream>
#include <openssl/err.h>
#include <cstring>

bool PacketIO_Server::sendAll(SSL* ssl, const void* data, const size_t size) {
    size_t total = 0;
    while (total < size) {
        const int sent = SSL_write(ssl, static_cast<const char*>(data) + total, static_cast<int>(size - total));
        if (sent <= 0) return false;
        total += sent;
    }

    return true;
}

bool PacketIO_Server::sendPacket(SSL* ssl, const std::string& data) {
    const std::string framed = frame(data);
    return sendAll(ssl, framed.data(), framed.size());
}

std::string PacketIO_Server::frame(const std::string& data) {
    const uint32_t len = htonl(static_cast<uint32_t>(data.size()));

    std::string framed;
    framed.reserve(sizeof(len) + data.size());
    framed.append(reinterpret_cast<const char*>(&len), sizeof(len));
    framed.append(data);
    return framed;
}

PacketIO_Server::Extract PacketIO_Server::extractPacket(std::string& buffer, std::string& packet) {
    uint32_t len;
    if (buffer.size() < sizeof(len)) return Extract::Incomplete;

    std::memcpy(&len, buffer.data(), sizeof(len));
    len = ntohl(len);

    if (len == 0 || len > MAX_PACKET_SIZE) {
        std::cerr << "Invalid packet size: " << len << std::endl;
        return Extract::Invalid;
    }

    if (buffer.size() < sizeof(len) + len) return Extract::Incomplete;

    packet.assign(buffer, sizeof(len), len);
    buffer.erase(0, sizeof(len) + len);
    return Extract::Packet;
}

bool PacketIO_Server::recvAll(SSL* ssl, void* data, const size_t size) {
    size_t total = 0;
    while (total < size) {
        const int bytes = SSL_read(ssl, static_cast<char*>(data) + total, static_cast<int>(size - total));
        if (bytes <= 0) {
            const int err = SSL_get_error(ssl, bytes);
            if (err == SSL_ERROR_ZERO_RETURN) {
                std::cerr << "Client was shutdown the connection\n";
                return false;
            }
            if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
                std::cerr << "Retry later\n";
                return false;
            }

            ERR_print_errors_fp(stderr);
            return false;
        }

        total += bytes;
    }

    return true;
}

bool PacketIO_Server::recvPacket(SSL* ssl, std::string& data) {
    uint32_t len;
    if (!recvAll(ssl, &len, sizeof(len))) return false;

    len = ntohl(len);

    if (len == 0 || len > MAX_PACKET_SIZE) {
        std::cerr << "Invalid packet size: " << len << std::endl;
        return false;
    }

    data.resize(len);

    if (!recvAll(ssl, data.data(), len)) return false;

    return true;
}