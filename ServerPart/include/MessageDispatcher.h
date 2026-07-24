#pragma once

#include "../include/SessionManager.h"
#include <functional>

class MessageDispatcher {
public:
    explicit MessageDispatcher(SessionManager& sm);

    std::function<void(std::shared_ptr<ClientSession>)> onDisconnect;

    void sendTo(const std::shared_ptr<ClientSession>& client, const std::string& msg) const;

    void broadcast(const std::string& msg) const;

private:
    SessionManager& sessionManager;
};