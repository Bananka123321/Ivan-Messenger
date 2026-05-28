#pragma once
#include <pqxx/pqxx>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <argon2.h>
#include <random>

#include "protocol.h"

class UserManager {
    struct AuthResult {
        bool success;
        int user_id;
        std::string error;
    };

public:
    UserManager(const std::string& conn_str) : conn(conn_str) {}
    AuthResult registerUser(const std::string& username, const std::string& password);
    AuthResult loginUser(const std::string& username, const std::string& password);
    bool bUsernameAvailable(const std::string& username);
    std::vector<User> searchUsers(const std::string& query);

private:
    std::string hashPassword(const std::string& password);
    pqxx::connection conn;
};