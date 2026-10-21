#include "DB_TemporaryTokenManager.h"
#include "../include/Logger.h"

DB_TemporaryTokenManager::DB_TemporaryTokenManager(DB_ConnectionPool& pool) : pool_(pool) {
    LOG_INFO(DB, "Инициализация DB_TemporaryTokenManager");
};

void DB_TemporaryTokenManager::createSession(int user_id, const std::string& token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        txn.exec("INSERT INTO connect_tokens (user_id, token) VALUES($1, $2);", pqxx::params(user_id, token));
        txn.commit();

        LOG_INFO(DB, "Создан временный токен подключения. userId=", user_id, " token=", token.substr(0, 8), "...");
    } catch(const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка создания временного токена. userId=", user_id, " ошибка=", e.what());
    }
}

bool DB_TemporaryTokenManager::isValid(const std::string& token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec(
            "SELECT * FROM connect_tokens WHERE token = $1;", pqxx::params(token));

        if (result.empty()) {
            LOG_DEBUG(DB, "Временный токен не найден. token=", token.substr(0, 8), "...");
            return false;
        }

        LOG_DEBUG(DB, "Временный токен валиден. token=", token.substr(0, 8), "...");
        return true;
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка проверки временного токена. token=", token.substr(0, 8), "... ошибка=", e.what());
        return false;
    }
}

std::optional<int> DB_TemporaryTokenManager::getUserIdByToken(const std::string& token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto res = txn.exec("SELECT user_id FROM connect_tokens WHERE token = $1;", pqxx::params(token));
        if(res.empty()) {
            LOG_DEBUG(DB, "Временный токен не найден при поиске userId. token=", token.substr(0, 8), "...");
            return std::nullopt;
        }
        int userId = res[0]["user_id"].as<int>();
        LOG_DEBUG(DB, "Найден userId по временному токену. token=", token.substr(0, 8), "... userId=", userId);
        return userId;
    } catch(const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка получения userId по временному токену. token=", token.substr(0, 8), "... ошибка=", e.what());
        return std::nullopt;
    }
}

void DB_TemporaryTokenManager::deleteSession(const std::string& token) {
    try {
        auto conn = pool_.acquire();
        pqxx::work txn(*conn);
        const auto result = txn.exec("DELETE FROM connect_tokens WHERE token = $1;", pqxx::params(token));
        txn.commit();

        LOG_INFO(DB, "Временный токен удалён. token=", token.substr(0, 8), "... удалено записей=", result.affected_rows());
    } catch (const std::exception& e) {
        LOG_CRITICAL(DB, "Ошибка удаления временного токена. token=", token.substr(0, 8), "... ошибка=", e.what());
    }
}