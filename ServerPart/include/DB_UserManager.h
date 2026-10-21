#pragma once
#include <pqxx/pqxx>
#include "DB_ConnectionPool.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <argon2.h>
#include <random>
#include <optional>

#include "protocol.h"

class DB_UserManager {
    struct AuthResult {
        bool success;
        int user_id;
        std::string error;
    };

public:
    explicit DB_UserManager(DB_ConnectionPool& pool);

    AuthResult registerUser(const std::string& username, const std::string& password);
    AuthResult loginUser(const std::string& username, const std::string& password);
    bool bUsernameAvailable(const std::string& username);
    std::vector<User> searchUsers(const std::string& query);
    std::optional<std::string> getUsername(int user_id);

private:
    static std::string hashPassword(const std::string& password);

private:
    DB_ConnectionPool& pool_;
};