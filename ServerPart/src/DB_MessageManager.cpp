#include "../include/DB_MessageManager.h"
#include <iostream>

messageManager::messageManager(const std::string& conn_str) : conn(conn_str) {};

int messageManager::saveMessage(int sender_id, int receiver_id, const std::string& text, int64_t timestamp) {
    try {
        pqxx::work txn(conn);
        auto result = txn.exec_params(
            "INSERT INTO messages (sender_id, receiver_id, text, timestamp)"
            " VALUES ($1, $2, $3, $4) RETURNING id", sender_id, receiver_id, text, timestamp);

        txn.commit();
        return result[0][0].as<int>();
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    }
};

std::vector<Message> messageManager::getHistory(int user_a, int user_b, int last_msg_id, int limit) {
    std::vector<Message> result;
    try {
        pqxx::work txn(conn);
        auto rows = txn.exec_params(
            "SELECT id, sender_id, receiver_id, text, timestamp FROM messages"
            " WHERE ((sender_id = $1 AND receiver_id = $2) OR (sender_id = $2 AND receiver_id = $1)) AND id < $3"
            " ORDER BY id LIMIT $4",
            user_a, user_b, last_msg_id, limit
        );

        for (const auto& row : rows) {
            result.push_back({
                row["id"].as<int>(),
                row["sender_id"].as<int>(),
                row["receiver_id"].as<int>(),
                row["text"].as<std::string>(),
                row["timestamp"].as<int64_t>()
            });
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';

    }
    return result;
};