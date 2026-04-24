#include "PacketIO.h"

bool PacketIO::sendAll(int sock, const void* data, size_t size) {
    size_t total = 0;
    ssize_t sent;
    while (total < size) {
        sent = send(sock, (char*)data + total, size - total, 0);
        if (sent <= 0) return false;
        total += sent;
    }

    return true;
}

bool PacketIO::sendPacket(int sock, const std::string& data) {
    uint32_t len = htonl(data.size());

    if (data.size() > MAX_PACKET_SIZE || data.size() == 0) {
        std::cerr << "Packet too big or too small\n";
        return false;
    }

    if (!sendAll(sock, &len, sizeof(len)))
        return false;

    if (!sendAll(sock, data.data(), data.size()))
        return false;

    return true;
}

bool PacketIO::recvAll(int sock, void* data, size_t size) {
    size_t total = 0;
    ssize_t bytes;
    while (total < size) {
        bytes = recv(sock, (char*)data + total, size - total, 0);
        if (bytes <= 0) return false;
        total += bytes;
    }

    return true;
}

bool PacketIO::recvPacket(int sock, std::string& data) {
    uint32_t len;
    if (!recvAll(sock, &len, sizeof(len))) return false;

    len = ntohl(len);

    if (len == 0 || len > MAX_PACKET_SIZE) {
        std::cerr << "Invalid packet size: " << len << std::endl;
        return false;
    }

    data.resize(len);

    if (!recvAll(sock, data.data(), len)) return false;

    return true;
}
