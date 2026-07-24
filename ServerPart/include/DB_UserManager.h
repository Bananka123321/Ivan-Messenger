#pragma once
#include <pqxx/pqxx>
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
    explicit DB_UserManager(const std::string& conn_str) : conn_(conn_str) {}

    AuthResult registerUser(const std::string& username, const std::string& password);
    AuthResult loginUser(const std::string& username, const std::string& password);
    bool bUsernameAvailable(const std::string& username);
    std::vector<User> searchUsers(const std::string& query);

    void createSession(int user_id, const std::string& token);
    std::optional<int> getUserIdByToken(const std::string& token);
    void deleteSession(const std::string& token);

private:
    static std::string hashPassword(const std::string& password);

private:
    pqxx::connection conn_;
};