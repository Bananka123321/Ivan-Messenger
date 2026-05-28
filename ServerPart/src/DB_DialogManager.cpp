#include "../include/DB_DialogManager.h"

DB_DialogManager::DB_DialogManager(const std::string& conn_str) : conn(conn_str) {}

void DB_DialogManager::insertDialog(int user_id, int peer_id, int msg_id, const std::string& text, int64_t timestamp) {
    try {
        pqxx::work txn(conn);
        txn.exec_params(
            "INSERT INTO dialogs (user_id, peer_id, last_msg_id, last_msg_text, last_msg_timestamp, updated_at)"
            " VALUES ($1, $2, $3, $4, $5, NOW())"
            " ON CONFLICT (user_id, peer_id) DO UPDATE SET"
            " last_msg_id = EXCLUDED.last_msg_id,"
            " last_msg_text = EXCLUDED.last_msg_text,"
            " last_msg_timestamp = EXCLUDED.last_msg_timestamp,"
            " updated_at = NOW()",
            user_id, peer_id, msg_id, text, timestamp
        );
        txn.commit();
    } catch(const std::exception& e) {
        std::cerr << "INSERT dialogs ERROR: " << e.what() << '\n';
    }
    
}

std::vector<MetaDialog> DB_DialogManager::getUserDialogs(int user_id) {
    std::vector<MetaDialog> result;
    try {
        pqxx::work txn(conn);
        auto rows = txn.exec_params(
            "SELECT d.peer_id, u.username, d.last_msg_text, d.last_msg_timestamp"
            " FROM dialogs d"
            " JOIN users u ON d.peer_id = u.id"
            " WHERE d.user_id = $1"
            " ORDER BY d.updated_at DESC",
            user_id
        );

        for (const auto& row : rows) {
            result.push_back({
                row["peer_id"].as<int>(),
                row["username"].as<std::string>(),
                row["last_msg_text"].as<std::string>(),
                row["last_msg_timestamp"].as<int64_t>()
            });
        }
    } catch(const std::exception& e) {
        std::cerr << "GET dialogs ERROR: " << e.what() << '\n';
    }
    return result;
}