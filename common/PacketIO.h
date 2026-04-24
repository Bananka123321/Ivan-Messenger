#pragma once

#include <string>
#include <iostream>
#include <netinet/in.h>

const uint32_t MAX_PACKET_SIZE = 1024*1024;

class PacketIO {
public:
    static bool sendPacket(int sock, const std::string& data);
    static bool recvPacket(int sock, std::string& data);

private:
    static bool sendAll(int sock, const void* data, size_t size);
    static bool recvAll(int sock, void* buffer, size_t size);
};