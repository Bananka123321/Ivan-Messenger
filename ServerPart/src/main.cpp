#include "../include/TCPServer.h"

int main() {
    TCPServer server(6767);
    if (!server.start()) {
        std::cerr << "Server failed to start\n";
        return 1;
    }
    return 0;
}