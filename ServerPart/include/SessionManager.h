#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <optional>

#include "ClientSession.h"

class SessionManager {
public:
    void add(std::shared_ptr<ClientSession> client);
    void remove(std::shared_ptr<ClientSession> client);

    std::shared_ptr<ClientSession> getByUserId(const int& user_id);
    std::vector<std::shared_ptr<ClientSession>> getAll();

    void updateActivity(std::shared_ptr<ClientSession> client);
    std::optional<int> checkToken(const std::string& token);

private:
    std::vector<std::shared_ptr<ClientSession>> sessions;
    std::mutex mutex;
};