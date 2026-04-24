#include "../include/UserManager.h"

bool UserManager::bUsernameAvailable(const std::string& username) {
    try {
        pqxx::connection C(conn_str);
        pqxx::work txn(C);
        pqxx::result r = txn.exec("SELECT id FROM users WHERE username = " + txn.quote(username));
        return r.empty();
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}

UserManager::AuthResult UserManager::registerUser(const std::string& username, const std::string& password) {
    if (!bUsernameAvailable(username)) return {false, -1, "Username is already taken"};

    std::string hashed = hashPassword(password);
    try {
        pqxx::connection C(conn_str);
        pqxx::work txn(C);
        pqxx::result r = txn.exec("INSERT INTO users(username, password_hash) VALUES (" + txn.quote(username) + ", " + txn.quote(hashed) + ") RETURNING id");
        txn.commit();
        return {true, r[0]["id"].as<int>(), ""};
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return {false, -1, e.what()};
    }
}

UserManager::AuthResult UserManager::loginUser(const std::string& username, const std::string& password) {
    std::string hashed = hashPassword(password);
    try {
        pqxx::connection C(conn_str);
        pqxx::work txn(C);
        pqxx::result r = txn.exec("SELECT id, password_hash FROM users WHERE username = " + txn.quote(username));
        if (r.empty()) return {false, -1, "User not found"};
        
        std::string dbHash = r[0]["password_hash"].c_str();
        if (dbHash != hashed) return {false, -1, "Invalid password"};
        return {true, r[0]["id"].as<int>(), ""};

    } catch(const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return {false, -1, e.what()};
    }
}

std::string UserManager::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.c_str(), password.size(), hash);
    std::stringstream ss;
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}