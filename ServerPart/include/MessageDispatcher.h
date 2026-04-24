#pragma once

#include "../include/SessionManager.h"
#include <functional>

class MessageDispatcher {
public:
    MessageDispatcher(SessionManager& sm);

    std::function<void(std::shared_ptr<ClientSession>)> onDisconnect;

    void sendTo(std::shared_ptr<ClientSession> client, const std::string& msg);

    void broadcast(const std::string& msg);

private:
    SessionManager& sessionManager;
};