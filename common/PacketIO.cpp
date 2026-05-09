#include "PacketIO.h"

bool PacketIO::sendAll(SSL* ssl, const void* data, size_t size) {
    size_t total = 0;
    while (total < size) {
        int sent = SSL_write(ssl, (char*)data + total, size - total);
        if (sent <= 0) return false;
        total += sent;
    }

    return true;
}

bool PacketIO::sendPacket(SSL* ssl, const std::string& data) {
    uint32_t len = htonl(data.size());

    if (!sendAll(ssl, &len, sizeof(len)))
        return false;

    if (!sendAll(ssl, data.data(), data.size()))
        return false;

    return true;
}

bool PacketIO::recvAll(SSL* ssl, void* data, size_t size) {
    size_t total = 0;
    while (total < size) {
        int bytes = SSL_read(ssl, (char*)data + total, size - total);
        if (bytes <= 0) {
            int err = SSL_get_error(ssl, bytes);
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

bool PacketIO::recvPacket(SSL* ssl, std::string& data) {
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
