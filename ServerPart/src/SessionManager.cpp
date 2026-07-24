#include "../include/SessionManager.h"

void SessionManager::add(const std::shared_ptr<ClientSession>& client){
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.push_back(client);
}

void SessionManager::remove(const std::shared_ptr<ClientSession>& client) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::erase(sessions_, client);
}

std::shared_ptr<ClientSession> SessionManager::getByUserId(const int& user_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& s : sessions_)
        if (s->getUserId() == user_id) return s;
    return nullptr;
}

std::vector<std::shared_ptr<ClientSession>> SessionManager::getAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    return sessions_;
}

void SessionManager::updateActivity(const std::shared_ptr<ClientSession>& client) {
    const int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    client->setLastActivity(now);
}