#include "../include/DB_MessageManager.h"
#include "../include/Logger.h"

DB_MessageManager::DB_MessageManager(DB_ConnectionPool& pool) : pool_(pool) {
    LOG_INFO(DB, "Инициализация DB_MessageManager");
};

std::optional<int64_t> DB_MessageManager::saveMessage(const Message& message) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        std::string payload_json = serializePayload(message.payload);
        const auto result = txn.exec(
            "INSERT INTO messages (dialog_id, sender_id, type, created_at, payload)"
            " VALUES ($1, $2, $3, to_timestamp($4 / 1000.0), $5::jsonb) RETURNING id",
            pqxx::params(message.dialog_id, message.sender_id, static_cast<int>(message.type), message.created_at_ms, payload_json));

        txn.commit();

        auto msgId = result[0][0].as<int64_t>();
        LOG_DEBUG(DB, "Сообщение сохранено. msgId=", msgId, " dialogId=", message.dialog_id, " senderId=", message.sender_id, " type=", static_cast<int>(message.type));

        return msgId;
    } catch(const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка сохранения сообщения. dialogId=", message.dialog_id, " senderId=", message.sender_id, " ошибка=", e.what());
        return std::nullopt;
    }
};

std::vector<Message> DB_MessageManager::getHistory(int64_t dialog_id, int64_t before_id, int limit) {
    std::vector<Message> result;
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto rows = txn.exec(
        "SELECT id, dialog_id, sender_id, type, "
        "(EXTRACT(EPOCH FROM created_at) * 1000)::bigint as timestamp_ms, "
        "payload::text "
        "FROM messages "
        "WHERE dialog_id = $1 AND id < $2 "
        "ORDER BY id DESC LIMIT $3",
            pqxx::params(dialog_id, before_id, limit)
        );

        result.reserve(rows.size());

        for (const auto& row : rows) {
            Message msg;
            msg.id = row["id"].as<int64_t>();
            msg.dialog_id = row["dialog_id"].as<int>();
            msg.sender_id = row["sender_id"].as<int>();
            msg.type = static_cast<MessageType>(row["type"].as<int>());
            msg.created_at_ms = row["timestamp_ms"].as<int64_t>();

            auto payload_json = row["payload"].as<std::string>();
            msg.payload = deserializePayload(payload_json, msg.type);

            result.push_back(std::move(msg));
        }

        LOG_INFO(DB, "Загружена история диалога. dialogId=", dialog_id, " beforeId=", before_id, " запрошено=", limit, " получено=", result.size());
    } catch(const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка загрузки истории. dialogId=", dialog_id, " beforeId=", before_id, " ошибка=", e.what());
    }
    return result;
};

std::string DB_MessageManager::serializePayload(const MessagePayload& payload) {
    return std::visit([]<typename T0>(const T0& content) -> std::string {
        using T = std::decay_t<T0>;

        nlohmann::json j;

        if constexpr (std::is_same_v<T, TextContent>) {
            j["text"] = content.text;
        }
        else if constexpr (std::is_same_v<T, MediaContent>) {
            j["url"] = content.url;
            j["filename"] = content.filename;
            j["size_bytes"] = content.size_bytes;
            j["caption"] = content.caption;
        }
        else if constexpr (std::is_same_v<T, VoiceContent>) {
            j["url"] = content.url;
            j["duration_sec"] = content.duration_sec;
        }

        return j.dump();
    }, payload);
}

MessagePayload DB_MessageManager::deserializePayload(const std::string& json, const MessageType type) {
    try {
        auto j = nlohmann::json::parse(json);

        switch (type) {
            case MessageType::Text:
                return TextContent{
                .text = j["text"].get<std::string>()
            };

            case MessageType::Image:
            case MessageType::File:
                return MediaContent{
                .url = j["url"].get<std::string>(),
                .caption = j.value("caption", ""),
                .filename = j.value("filename", ""),
                .size_bytes = j.value("size_bytes", 0ULL)
            };

            case MessageType::Voice:
                return VoiceContent{
                .url = j["url"].get<std::string>(),
                .duration_sec = j["duration_sec"].get<uint32_t>()
            };

            default:
                LOG_WARNING(DB, "Неизвестный тип сообщения при десериализации. type=", static_cast<int>(type));
                throw std::runtime_error("Unknown message type");
        }
    } catch (const nlohmann::json::exception& e) {
        LOG_CRITICAL(DB, "Ошибка парсинга payload. type=", static_cast<int>(type), " ошибка=", e.what(), " данные=", json.substr(0, 100));
        throw;
    }
}