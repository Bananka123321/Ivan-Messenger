#include "DB_DevicesSessionManager.h"
#include "../include/Logger.h"

#include <random>
#include "iostream"

DB_DevicesSessionManager::DB_DevicesSessionManager(DB_ConnectionPool& pool) : pool_(pool) {
    LOG_INFO(DB, "Инициализация DB_DevicesSessionManager");
};

std::optional<std::string> DB_DevicesSessionManager::createToken(const int user_id, const std::string &device_info, const std::string &ip_address) {
    try {
        std::lock_guard<std::mutex> lock(rng_mutex);

        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
        std::stringstream ss;
        for(int i = 0; i < 8; i++) {
            ss << std::hex << std::setw(8) << std::setfill('0') << dis(gen);
        }

        std::string token = ss.str();

        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        txn.exec(
            "INSERT INTO user_sessions (user_id, token, device_info, ip_address) "
            "VALUES ($1, $2, $3, $4);",
            pqxx::params(user_id, token, device_info, ip_address));
        txn.commit();

        LOG_INFO(DB, "Создан новый токен сессии. userId=", user_id, " token=", token.substr(0, 8), "...", " ip=", ip_address);

        return token;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка создания токена сессии. userId=", user_id, " ошибка=", e.what());
        return std::nullopt;
    }
}

bool DB_DevicesSessionManager::isValid(const std::string& token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec(
            "SELECT * FROM user_sessions WHERE token = $1 AND "
            "expires_at > CURRENT_TIMESTAMP;"
            , pqxx::params(token));

        if (!result.empty()) {
            LOG_DEBUG(DB, "Токен валиден. token=", token.substr(0, 8), "...");
            return true;
        }

        const auto check = txn.exec(
            "SELECT 1 FROM user_sessions WHERE token = $1",
            pqxx::params(token));

        if (!check.empty()) {
            LOG_INFO(DB, "Обнаружен просроченный токен. Удаление. token=", token.substr(0, 8), "...");
            txn.exec("DELETE FROM user_sessions WHERE token = $1", pqxx::params(token));
            txn.commit();
        } else {
            LOG_DEBUG(DB, "Токен не найден в БД. token=", token.substr(0, 8), "...");
        }

        return false;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка проверки валидности токена. token=", token.substr(0, 8), "... ошибка=", e.what());
        return false;
    }
}

void DB_DevicesSessionManager::updateActivity(const std::string& token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        txn.exec("UPDATE user_sessions SET last_activity = CURRENT_TIMESTAMP "
                 "WHERE token = $1;",
                 pqxx::params(token));
        txn.commit();
        LOG_DEBUG(DB, "Обновлена активность сессии. token=", token.substr(0, 8), "...");
    } catch (const std::exception& e) {
        LOG_WARNING(DB, "Не удалось обновить активность. token=", token.substr(0, 8), "... ошибка=", e.what());
    }
}

void DB_DevicesSessionManager::deleteSession(const std::string &token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        txn.exec("DELETE FROM user_sessions WHERE token = $1;", pqxx::params(token));
        txn.commit();
        LOG_INFO(DB, "Сессия удалена. token=", token.substr(0, 8), "...");
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка удаления сессии. token=", token.substr(0, 8), "... ошибка=", e.what());
    }
}

void DB_DevicesSessionManager::deleteAllSessions(int user_id, const std::string &tokenSafe) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        auto result = txn.exec("DELETE FROM user_sessions WHERE user_id = $1 AND token != $2;",
            pqxx::params(user_id, tokenSafe));
        txn.commit();

        LOG_INFO(DB, "Удалены все сессии пользователя. userId=", user_id, " удалено записей=", result.affected_rows(), " сохранен токен=", tokenSafe.substr(0, 8), "...");
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка массового удаления сессий. userId=", user_id, " ошибка=", e.what());
    }
}

std::optional<int> DB_DevicesSessionManager::getUserIdByToken(const std::string& token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec("SELECT user_id FROM user_sessions WHERE token = $1 "
                                     "AND expires_at > CURRENT_TIMESTAMP;",
            pqxx::params(token));
        if (result.empty()) {
            LOG_DEBUG(DB, "Токен не найден при поиске userId. token=", token.substr(0, 8), "...");
            return std::nullopt;
        }
        int userId = result[0]["user_id"].as<int>();
        LOG_DEBUG(DB, "Найден userId по токену. token=", token.substr(0, 8), "... userId=", userId);
        return userId;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка получения userId по токену. token=", token.substr(0, 8), "... ошибка=", e.what());
        return std::nullopt;
    }
}