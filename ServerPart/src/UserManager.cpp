#include "../include/UserManager.h"

bool UserManager::bUsernameAvailable(const std::string& username) {
    try {
        pqxx::work txn(conn);
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
        pqxx::work txn(conn);
        pqxx::result r = txn.exec("INSERT INTO users(username, password_hash) VALUES (" + txn.quote(username) + ", " + txn.quote(hashed) + ") RETURNING id");
        txn.commit();
        return {true, r[0]["id"].as<int>(), ""};
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return {false, -1, e.what()};
    }
}

UserManager::AuthResult UserManager::loginUser(const std::string& username, const std::string& password) {
    try {
        pqxx::work txn(conn);
        pqxx::result r = txn.exec("SELECT id, password_hash FROM users WHERE username = " + txn.quote(username));
        if (r.empty()) return {false, -1, "User not found"};
        
        std::string dbHash = r[0]["password_hash"].c_str();
        
        if (argon2id_verify(dbHash.c_str(), password.c_str(), password.size()) == ARGON2_OK)
            return {true, r[0]["id"].as<int>(), ""};
        return {false, -1, "Invalid password"};

    } catch(const std::exception &e) {
        std::cerr << e.what() << '\n';
        return {false, -1, e.what()};
    }
}

std::string UserManager::hashPassword(const std::string& password) {
    char salt[16];

    std::random_device rd;
    for (size_t i = 0; i < sizeof(salt); i++)
        salt[i] = static_cast<unsigned char>(rd());

    char hash[128] = {0};
    try {
        int result = argon2id_hash_encoded(3, 65536, 4, password.c_str(), password.size(), salt, sizeof(salt), 32, hash, sizeof(hash));

        if (result != ARGON2_OK) {
            std::cerr << "Argon2 hashing failed: " << argon2_error_message(result) << '\n';
            return "";
        }

        return hash;

    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return "";
    }
}

std::vector<protocol::User> UserManager::searchUsers(const std::string& query) {
    std::vector<protocol::User> result;

    try {
        pqxx::work txn(conn);

        pqxx::result r = txn.exec("SELECT id, username FROM users WHERE username ILIKE " + txn.quote(query + "%") + " LIMIT 20");

        for (auto row : r)
            result.push_back({row["id"].as<int>(), row["username"].c_str()});

        return result;

    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return {};
    }
}