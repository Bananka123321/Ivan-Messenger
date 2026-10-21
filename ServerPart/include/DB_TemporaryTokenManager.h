#pragma once
#include <string>
#include <optional>
#include <pqxx/pqxx>
#include "DB_ConnectionPool.h"
#include <iostream>

class DB_TemporaryTokenManager {
public:
    explicit DB_TemporaryTokenManager(DB_ConnectionPool& pool);

    void createSession(int user_id, const std::string& token);
    std::optional<int> getUserIdByToken(const std::string& token);
    void deleteSession(const std::string& token);
    bool isValid(const std::string& token);

private:
    DB_ConnectionPool& pool_;
};