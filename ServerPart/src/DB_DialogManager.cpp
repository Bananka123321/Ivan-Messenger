#include "../include/DB_DialogManager.h"
#include "../include/Logger.h"

DB_DialogManager::DB_DialogManager(DB_ConnectionPool& pool) : pool_(pool) {
    LOG_INFO(DB, "Инициализация DB_DialogManager");
}

void DB_DialogManager::upsertDialog(int user_id, int peer_id, int64_t msg_id, const std::string& preview, int64_t timestamp) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        txn.exec(
            "INSERT INTO dialogs (user_id, peer_id, last_msg_id, last_msg_text, last_msg_timestamp, updated_at) "
            "VALUES ($1, $2, $3, $4, to_timestamp($5 / 1000.0), NOW()) "
            "ON CONFLICT (user_id, peer_id) DO UPDATE SET "
            "last_msg_id = EXCLUDED.last_msg_id, "
            "last_msg_text = EXCLUDED.last_msg_text, "
            "last_msg_timestamp = EXCLUDED.last_msg_timestamp, "
            "updated_at = NOW()",
            pqxx::params(user_id, peer_id, msg_id, preview, timestamp)
        );

        txn.commit();

        LOG_DEBUG(DB, "Upsert диалога. userId=", user_id, " peerId=", peer_id, " msgId=", msg_id);
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка upsert диалога. userId=", user_id, " peerId=", peer_id, " msgId=", msg_id, " ошибка=", e.what());
    }
}

void DB_DialogManager::updateLastMessage(const Message &msg) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        txn.exec(
            "INSERT INTO dialog_last_message (dialog_id, last_msg_id, last_msg_text, last_msg_sender_id, last_msg_timestamp)"
            " VALUES ($1, $2, $3, $4, to_timestamp($5 / 1000.0))"
            " ON CONFLICT (dialog_id) DO UPDATE SET"
            " last_msg_id = EXCLUDED.last_msg_id,"
            " last_msg_text = EXCLUDED.last_msg_text,"
            " last_msg_sender_id = EXCLUDED.last_msg_sender_id,"
            " last_msg_timestamp = EXCLUDED.last_msg_timestamp",
            pqxx::params(msg.dialog_id, msg.id, msg.getPreview(), msg.sender_id, msg.created_at_ms)
        );
        txn.commit();

        LOG_DEBUG(DB, "Обновлено последнее сообщение диалога. dialogId=", msg.dialog_id, " msgId=", msg.id, " senderId=", msg.sender_id);
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка обновления последнего сообщения. dialogId=", msg.dialog_id, " msgId=", msg.id, " ошибка=", e.what());
    }
}

std::vector<MetaDialog_Client> DB_DialogManager::getUserDialogs(int user_id) {
    std::vector<MetaDialog_Client> result;
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        const auto rows = txn.exec(
            "SELECT "
            "    d.id AS dialog_id, "
            "    dp_peer.user_id AS peer_id, "
            "    u_peer.username AS peer_username, "
            "    dlm.last_msg_text, "
            "    (EXTRACT(EPOCH FROM dlm.last_msg_timestamp) * 1000)::bigint AS last_msg_timestamp, "
            "    dlm.last_msg_sender_id, "
            "    CASE "
            "        WHEN dlm.last_msg_sender_id = $1 THEN 'Вы' "
            "        ELSE u_peer.username "
            "    END AS display_name, "
            "    (SELECT COUNT(*) FROM messages m "
            "     WHERE m.dialog_id = d.id AND m.id > dp.last_read_msg_id) AS unread_count "
            "FROM dialog_participants dp "
            "JOIN dialogs d ON dp.dialog_id = d.id "
            "JOIN dialog_participants dp_peer "
            "    ON d.id = dp_peer.dialog_id AND dp_peer.user_id != $1 "
            "JOIN users u_peer ON u_peer.id = dp_peer.user_id "
            "LEFT JOIN dialog_last_message dlm ON d.id = dlm.dialog_id "
            "WHERE dp.user_id = $1 "
            "ORDER BY d.updated_at DESC NULLS LAST",
            pqxx::params(user_id)
        );

        result.reserve(rows.size());

        for (const auto& row : rows) {
            MetaDialog_Client md;
            md.dialog_id = row["dialog_id"].as<int64_t>();
            md.peer_id = row["peer_id"].as<int>();
            md.username = row["peer_username"].as<std::string>();
            md.display_name = row["display_name"].as<std::string>();
            md.last_msg_preview = row["last_msg_text"].as<std::string>("");
            md.last_msg_timestamp = row["last_msg_timestamp"].as<int64_t>(0);
            md.unread_count = row["unread_count"].as<int>();
            result.push_back(std::move(md));
        }

        LOG_INFO(DB, "Загружены диалоги пользователя. userId=", user_id, " количество=", result.size());
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка загрузки диалогов пользователя. userId=", user_id, " ошибка=", e.what());
    }
    return result;
}

std::vector<int> DB_DialogManager::getDialogParticipants(int64_t dialog_id) {
    std::vector<int> result;
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto& rows = txn.exec(
            "SELECT user_id FROM dialog_participants"
            " WHERE dialog_id = $1",
            pqxx::params(dialog_id)
            );

        for (const auto& row : rows) {
            result.push_back(row["user_id"].as<int>());
        }

        LOG_DEBUG(DB, "Получены участники диалога. dialogId=", dialog_id, " количество=", result.size());
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка получения участников диалога. dialogId=", dialog_id, " ошибка=", e.what());
    }
    return result;
}

std::optional<int64_t> DB_DialogManager::findOrCreatePrivateDialog(int user_a, int user_b) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);

        txn.exec("SELECT pg_advisory_xact_lock($1, $2)",
                 pqxx::params(std::min(user_a, user_b), std::max(user_a, user_b)));

        const auto existing = txn.exec(
            "SELECT dp1.dialog_id FROM dialog_participants dp1 "
            "JOIN dialog_participants dp2 ON dp2.dialog_id = dp1.dialog_id "
            "WHERE dp1.user_id = $1 AND dp2.user_id = $2 "
            "AND (SELECT COUNT(*) FROM dialog_participants dp3 WHERE dp3.dialog_id = dp1.dialog_id) = 2 "
            "LIMIT 1",
            pqxx::params(user_a, user_b));

        if (!existing.empty()) {
            const auto id = existing[0][0].as<int64_t>();
            txn.commit();
            LOG_DEBUG(DB, "Найден существующий диалог. dialogId=", id, " users=", user_a, ",", user_b);
            return id;
        }

        const auto created = txn.exec("INSERT INTO dialogs (updated_at) VALUES (NOW()) RETURNING id");
        const auto id = created[0][0].as<int64_t>();

        txn.exec(
            "INSERT INTO dialog_participants (dialog_id, user_id, last_read_msg_id) "
            "VALUES ($1, $2, 0), ($1, $3, 0)",
            pqxx::params(id, user_a, user_b));

        txn.commit();
        LOG_INFO(DB, "Создан новый диалог. dialogId=", id, " users=", user_a, ",", user_b);
        return id;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка поиска/создания диалога. users=", user_a, ",", user_b, " ошибка=", e.what());
        return std::nullopt;
    }
}