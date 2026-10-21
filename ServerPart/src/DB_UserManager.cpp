#include "../include/DB_UserManager.h"

DB_UserManager::DB_UserManager(DB_ConnectionPool& pool) : pool_(pool) {};

bool DB_UserManager::bUsernameAvailable(const std::string& username) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const pqxx::result r = txn.exec("SELECT id FROM users WHERE username = " + txn.quote(username));
        return r.empty();
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}

DB_UserManager::AuthResult DB_UserManager::registerUser(const std::string& username, const std::string& password) {
    if (!bUsernameAvailable(username)) {
        return {.success = false, .user_id = -1, .error = "Username is already taken"};
    }

    const std::string hashed = hashPassword(password);
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const pqxx::result r = txn.exec("INSERT INTO users(username, password_hash) VALUES (" + txn.quote(username) + ", " + txn.quote(hashed) + ") RETURNING id");
        txn.commit();
        return {.success = true, .user_id = r[0]["id"].as<int>(), .error = ""};
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return {.success = false, .user_id = -1, .error = e.what()};
    }
}

DB_UserManager::AuthResult DB_UserManager::loginUser(const std::string& username, const std::string& password) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const pqxx::result r = txn.exec("SELECT id, password_hash FROM users WHERE username = " + txn.quote(username));
        if (r.empty()) return {.success = false, .user_id = -1, .error = "User not found"};

        const std::string dbHash = r[0]["password_hash"].c_str();
        
        if (argon2id_verify(dbHash.c_str(), password.c_str(), password.size()) == ARGON2_OK)
            return {.success = true, .user_id = r[0]["id"].as<int>(), .error = ""};
        return {.success = false, .user_id = -1, .error = "Invalid password"};

    } catch(const std::exception &e) {
        std::cerr << e.what() << '\n';
        return {.success = false, .user_id = -1, .error = e.what()};
    }
}

std::string DB_UserManager::hashPassword(const std::string& password) {
    char salt[16];

    std::random_device rd;
    for (char & i : salt)
        i = static_cast<unsigned char>(rd());

    char hash[128] = {0};
    try {
        if (const int result = argon2id_hash_encoded(3, 65536, 4, password.c_str(), password.size(), salt, sizeof(salt), 32, hash, sizeof(hash)); result != ARGON2_OK) {
            std::cerr << "Argon2 hashing failed: " << argon2_error_message(result) << '\n';
            return "";
        }

        return hash;

    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return "";
    }
}

std::vector<User> DB_UserManager::searchUsers(const std::string& query) {
    try {
        std::vector<User> result;
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        for (const pqxx::result r = txn.exec("SELECT id, username FROM users WHERE username LIKE " + txn.quote(query + "%") + " LIMIT 20"); const auto& row : r)
            result.push_back({.username = row["username"].c_str(), .user_id = row["id"].as<int>()});

        return result;

    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return {};
    }
}

std::optional<std::string> DB_UserManager::getUsername(int user_id) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec("SELECT username FROM users WHERE id = $1;", pqxx::params(user_id));
        if (result.empty()) return std::nullopt;
        return result[0]["username"].as<std::string>();
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return std::nullopt;
    }
}