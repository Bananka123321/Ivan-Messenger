#include "../include/TCPServer.h"

int main() {
    if(auto error = Config::load("config.json")){
        std::cerr << "Failed to laod config: " << *error << '\n';
        return 1;
    }

    TCPServer server(Config::getServer().port);

    if (!server.start()) {
        std::cerr << "Server failed to start\n";
        return 1;
    }

    return 0;
}