#pragma once
#include <pqxx/pqxx>
#include <openssl/sha.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <iostream>

class UserManager {
    struct AuthResult {
        bool success;
        int user_id;
        std::string error;
    };

public:
    UserManager(const std::string& conn_str) : conn_str(conn_str) {}
    AuthResult registerUser(const std::string& username, const std::string& password);
    AuthResult loginUser(const std::string& username, const std::string& password);
    bool bUsernameAvailable(const std::string& username);

private:
    std::string conn_str;

    std::string hashPassword(const std::string& password);
};