#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <optional>

#include "ClientSession.h"

class SessionManager {
public:
    void add(const std::shared_ptr<ClientSession>& client);
    void remove(const std::shared_ptr<ClientSession>& client);

    std::shared_ptr<ClientSession> getByUserId(const int& user_id);
    std::vector<std::shared_ptr<ClientSession>> getAll();

    static void updateActivity(const std::shared_ptr<ClientSession>& client);
    std::optional<int> checkToken(const std::string& token);

private:
    std::vector<std::shared_ptr<ClientSession>> sessions_;
    std::mutex mutex_;
};