#include "../include/MessageDispatcher_.h"

MessageDispatcher_::MessageDispatcher_(SessionManager& sm) : sessionManager_(sm) {}

void MessageDispatcher_::sendTo(const std::shared_ptr<ClientSession>& client, const std::string& msg) const {
    if (!client->send(msg) && onDisconnect)
        onDisconnect(client);
}

void MessageDispatcher_::broadcast(const std::string& msg) const {
    for (const auto clients = sessionManager_.getAll(); auto& c : clients) {
        if (!c->send(msg) && onDisconnect)
            onDisconnect(c);
    }
}
