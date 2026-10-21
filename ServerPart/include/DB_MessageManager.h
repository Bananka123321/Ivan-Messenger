#pragma once
#include <pqxx/pqxx>
#include "DB_ConnectionPool.h"
#include <string>
#include <vector>
#include <optional>

#include "../common/Message.h"
#include "nlohmann/json.hpp"

class DB_MessageManager {
public:
    explicit DB_MessageManager(DB_ConnectionPool& pool);

    std::optional<int64_t> saveMessage(const Message& message);
    std::vector<Message> getHistory(int64_t dialog_id, int64_t before_id, int limit);
    
private:
    DB_ConnectionPool& pool_;

    static std::string serializePayload(const MessagePayload& payload);
    static MessagePayload deserializePayload(const std::string& json, MessageType type);
};