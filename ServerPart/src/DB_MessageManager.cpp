#include "../include/DB_MessageManager.h"
#include <iostream>

DB_MessageManager::DB_MessageManager(const std::string& conn_str) : conn_(conn_str) {};

int DB_MessageManager::saveMessage(int sender_id, int receiver_id, const std::string& text, int64_t timestamp) {
    try {
        pqxx::work txn(conn_);
        const auto result = txn.exec(
            "INSERT INTO messages (sender_id, receiver_id, text, timestamp)"
            " VALUES ($1, $2, $3, $4) RETURNING id",
            pqxx::params(sender_id, receiver_id, text, timestamp));

        txn.commit();
        return result[0][0].as<int>();
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    }
};

std::vector<Message> DB_MessageManager::getHistory(int user_a, int user_b, int last_msg_id, int limit) {
    std::vector<Message> result;
    try {
        pqxx::work txn(conn_);
        const auto rows = txn.exec(
            "SELECT id, sender_id, receiver_id, text, timestamp FROM messages"
            " WHERE ((sender_id = $1 AND receiver_id = $2) OR (sender_id = $2 AND receiver_id = $1)) AND id < $3"
            " ORDER BY id DESC LIMIT $4",
            pqxx::params(user_a, user_b, last_msg_id, limit)
        );

        for (const auto& row : rows) {
            result.push_back({
                .msgId = row["id"].as<int>(),
                .senderId = row["sender_id"].as<int>(),
                .receiverId = row["receiver_id"].as<int>(),
                .text = row["text"].as<std::string>(),
                .timestamp = row["timestamp"].as<int64_t>()
            });
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';

    }
    return result;
};