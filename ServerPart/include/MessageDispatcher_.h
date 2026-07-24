#pragma once

#include <functional>

#include "../include/SessionManager.h"

class MessageDispatcher_ {
public:
    explicit MessageDispatcher_(SessionManager& sm);

    std::function<void(std::shared_ptr<ClientSession>)> onDisconnect;

    void sendTo(const std::shared_ptr<ClientSession>& client, const std::string& msg) const;

    void broadcast(const std::string& msg) const;

private:
    SessionManager& sessionManager_;
};