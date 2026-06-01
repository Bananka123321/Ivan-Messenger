#include "../include/SessionManager.h"

void SessionManager::add(std::shared_ptr<ClientSession> client){
    std::lock_guard<std::mutex> lock(mutex);
    sessions.push_back(client);
}

void SessionManager::remove(std::shared_ptr<ClientSession> client) {
    std::lock_guard<std::mutex> lock(mutex);
    
    sessions.erase(std::remove(sessions.begin(), sessions.end(), client), sessions.end());
}

std::shared_ptr<ClientSession> SessionManager::getByUserId(const int& user_id) {
    std::lock_guard<std::mutex> lock(mutex);
    for (auto& s : sessions)
        if (s->getUserId() == user_id) return s;
    return nullptr;
}

std::vector<std::shared_ptr<ClientSession>> SessionManager::getAll() {
    std::lock_guard<std::mutex> lock(mutex);
    return sessions;
}

void SessionManager::updateActivity(std::shared_ptr<ClientSession> client) {
    int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    client->setLastActivity(now);
}

std::optional<int> SessionManager::checkToken(const std::string& token) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = tokens.find(token);
    if(it == tokens.end()) {
        std::cerr << "token was not find. Tokens.size = " << tokens.size() << '\n';
        for (const auto& [key, value] : tokens) {
            std::cerr << "KEY -- " << key << '\n';
        }
        
        return std::nullopt;
    }
    return it->second;
}

void SessionManager::addToken(const std::string& token, int user_id) {
    std::lock_guard<std::mutex> lock(mutex);
    tokens[token] = user_id;
    std::cerr << "token was added\n";
}