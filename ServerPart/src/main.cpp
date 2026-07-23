#include "../include/TCPServer.h"
#include "../include/SignalHandler.h"

int main() {
    SignalHandler::Setup();

    if(auto error = Config::load("config.json")){
        std::cerr << "Failed to laod config: " << *error << '\n';
        return 1;
    }

    TCPServer server(Config::getServer().port);

    std::thread serverThread([&server](){
        if (!server.start())
            std::cerr << "Server failed to start\n";
    });

    while(!SignalHandler::isShutdownRequested())
        pause();

    server.stop();

    if(serverThread.joinable())
        serverThread.join();

    return 0;
}