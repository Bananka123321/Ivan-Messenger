#pragma once

#include <string>
#include <openssl/ssl.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

constexpr uint32_t MAX_PACKET_SIZE = 1024*1024;

class PacketIO_Server {
public:
    enum class Extract { Incomplete, Packet, Invalid };

    static bool sendPacket(SSL* ssl, const std::string& data);
    static bool recvPacket(SSL* ssl, std::string& data);

    static std::string frame(const std::string& data);
    static Extract extractPacket(std::string& buffer, std::string& packet);

private:
    static bool sendAll(SSL* ssl, const void* data, size_t size);
    static bool recvAll(SSL* ssl, void* data, size_t size);
};