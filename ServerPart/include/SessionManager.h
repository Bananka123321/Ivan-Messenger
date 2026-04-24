#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <algorithm>
#include "ClientSession.h"

class SessionManager {
public:
    void add(std::shared_ptr<ClientSession> client);
    void remove(std::shared_ptr<ClientSession> client);

    std::shared_ptr<ClientSession> getByUsername(const std::string& username);
    std::vector<std::shared_ptr<ClientSession>> getAll();

private:
    std::vector<std::shared_ptr<ClientSession>> sessions;
    std::mutex mutex;
};