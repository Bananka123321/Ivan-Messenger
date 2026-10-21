#pragma once
#include <string>
#include <optional>
#include <mutex>
#include <random>
#include "pqxx/pqxx"
#include "DB_ConnectionPool.h"

class DB_DevicesSessionManager {
public:
    explicit DB_DevicesSessionManager(DB_ConnectionPool& pool);

    std::optional<std::string> createToken(int user_id, const std::string& device_info, const std::string& ip_address);
    bool isValid(const std::string& token);
    void updateActivity(const std::string& token);
    void deleteSession(const std::string& token);
    void deleteAllSessions(int user_id, const std::string& tokenSafe);
    std::optional<int> getUserIdByToken(const std::string& token);

private:
    DB_ConnectionPool& pool_;
    std::mutex rng_mutex;
    std::random_device rd;
};