#include "../include/DB_UserManager.h"

bool DB_UserManager::bUsernameAvailable(const std::string& username) {
    try {
        pqxx::work txn(conn_);
        const pqxx::result r = txn.exec("SELECT id FROM users WHERE username = " + txn.quote(username));
        return r.empty();
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return false;
    }
}

DB_UserManager::AuthResult DB_UserManager::registerUser(const std::string& username, const std::string& password) {
    if (!bUsernameAvailable(username))
        return {.success = false, .user_id = -1, .error = "Username is already taken"};

    const std::string hashed = hashPassword(password);
    try {
        pqxx::work txn(conn_);
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
        pqxx::work txn(conn_);
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
        pqxx::work txn(conn_);

        for (const pqxx::result r = txn.exec("SELECT id, username FROM users WHERE username LIKE " + txn.quote(query + "%") + " LIMIT 20"); const auto& row : r)
            result.push_back({.user_id = row["id"].as<int>(), .username = row["username"].c_str()});

        return result;

    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return {};
    }
}

void DB_UserManager::createSession(int user_id, const std::string& token) {
    try {
        pqxx::work txn(conn_);
        txn.exec("INSERT INTO user_sessions (user_id, token) VALUES($1, $2)", pqxx::params(user_id, token));
        txn.commit();
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
}

std::optional<int> DB_UserManager::getUserIdByToken(const std::string& token) {
    try {
        pqxx::work txn(conn_);
        const auto res = txn.exec("SELECT user_id FROM user_sessions WHERE token = $1", pqxx::params(token));
        if(res.empty()) return std::nullopt;
        return res[0]["user_id"].as<int>();
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return std::nullopt;
    }   
}

void DB_UserManager::deleteSession(const std::string& token) {
    try {
        pqxx::work txn(conn_);
        txn.exec("DELETE FROM user_sessions WHERE token = $1", pqxx::params(token));
        txn.commit();
    } catch (const std::exception& e) {
        std::cerr << "DB Error deleting session: " << e.what() << '\n';
    }
}