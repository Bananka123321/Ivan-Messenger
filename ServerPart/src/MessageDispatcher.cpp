#include "../include/MessageDispatcher.h"

MessageDispatcher::MessageDispatcher(SessionManager& sm) : sessionManager(sm) {}

void MessageDispatcher::sendTo(std::shared_ptr<ClientSession> client, const std::string& msg) {
    if (!client->send(msg) && onDisconnect)
        onDisconnect(client);
}

void MessageDispatcher::broadcast(const std::string& msg) {
    auto clients = sessionManager.getAll();
    for (auto& c : clients) {
        if (!c->send(msg) && onDisconnect)
            onDisconnect(c);
    }
}