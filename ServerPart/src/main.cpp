#include "../include/TcpServer.h"
#include "../include/SignalHandler.h"

int main() {
    SignalHandler::s_Setup();

    if(const auto error = Config::load("config.json")){
        std::cerr << "Failed to load config: " << *error << '\n';
        return 1;
    }

    TcpServer server(Config::getServer().port);

    std::thread serverThread([&server](){
        if (!server.start())
            std::cerr << "Server failed to start\n";
    });

    while(!SignalHandler::s_isShutdownRequested())
        pause();

    server.stop();

    if(serverThread.joinable())
        serverThread.join();

    return 0;
}