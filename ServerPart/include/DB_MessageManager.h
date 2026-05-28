#pragma once
#include <pqxx/pqxx>
#include <string>
#include <vector>

#include "../common/Message.h"

class messageManager {
public:
    explicit messageManager(const std::string& conn_str);

    int saveMessage(int sender_id, int receiver_id, const std::string& text, int64_t timestamp);
    std::vector<Message> getHistory(int user_a, int user_b, int last_msg_id, int limit);
    
private:
    pqxx::connection conn;
};