#pragma once

#include <string>
#include <iostream>
#include <netinet/in.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

const uint32_t MAX_PACKET_SIZE = 1024*1024;

class PacketIO {
public:
    static bool sendPacket(SSL* ssl, const std::string& data);
    static bool recvPacket(SSL* ssl, std::string& data);

private:
    static bool sendAll(SSL* ssl, const void* data, size_t size);
    static bool recvAll(SSL* ssl, void* data, size_t size);
};