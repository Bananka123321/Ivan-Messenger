#include "TestDatabase.h"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

namespace {

std::string env(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    return value && *value ? value : fallback;
}

bool databaseRequired() {
    return env("MESSENGER_TEST_DB_REQUIRED", "0") == "1";
}

}

const TestDatabase::Settings& TestDatabase::settings() {
    static const Settings s{
        .host = env("MESSENGER_TEST_DB_HOST", "127.0.0.1"),
        .port = std::stoi(env("MESSENGER_TEST_DB_PORT", "55432")),
        .name = env("MESSENGER_TEST_DB_NAME", "messenger_test"),
        .user = env("MESSENGER_TEST_DB_USER", "messenger"),
        .password = env("MESSENGER_TEST_DB_PASSWORD", "messenger"),
    };
    return s;
}

std::string TestDatabase::connectionString() {
    const auto& s = settings();
    return "dbname=" + s.name + " user=" + s.user + " password=" + s.password +
           " host=" + s.host + " port=" + std::to_string(s.port) + " connect_timeout=5";
}

std::unique_ptr<TempFile> TestDatabase::makeServerConfig() {
    const auto& s = settings();
    const nlohmann::json config = {
        {"database", {{"name", s.name}, {"user", s.user}, {"password", s.password}, {"host", s.host}, {"port", s.port}, {"timeout", 5}}},
        {"server", {{"port", 0}, {"max_connections", 10}}},
    };
    return std::make_unique<TempFile>(config.dump());
}

void TestDatabase::ensureSchema() {
    pqxx::connection conn(connectionString());
    {
        pqxx::work txn(conn);
        if (txn.query_value<bool>("SELECT to_regclass('public.users') IS NOT NULL"))
            return;
    }

    std::ifstream file(SCHEMA_SQL_FILE);
    if (!file)
        throw std::runtime_error("Не найден файл схемы: " SCHEMA_SQL_FILE);
    std::stringstream sql;
    sql << file.rdbuf();

    pqxx::work txn(conn);
    txn.exec(sql.str());
    txn.commit();
}

void TestDatabase::truncateAll() {
    pqxx::connection conn(connectionString());
    pqxx::work txn(conn);
    txn.exec("TRUNCATE users, dialogs, dialog_participants, dialog_last_message, messages, "
             "user_sessions, connect_tokens RESTART IDENTITY CASCADE");
    txn.commit();
}

void DatabaseTest::SetUpTestSuite() {
    unavailableReason_.clear();
    try {
        TestDatabase::ensureSchema();
    } catch (const std::exception& e) {
        unavailableReason_ = std::string("тестовая база недоступна (") + e.what() +
                             "). Запустите tests/scripts/test_db.sh start или ctest.";
    }
}

void DatabaseTest::SetUp() {
    if (!unavailableReason_.empty()) {
        if (databaseRequired())
            FAIL() << unavailableReason_;
        GTEST_SKIP() << unavailableReason_;
    }
    TestDatabase::truncateAll();
    db_ = std::make_unique<pqxx::connection>(TestDatabase::connectionString());
}

pqxx::connection& DatabaseTest::db() {
    return *db_;
}

pqxx::result DatabaseTest::exec(const std::string& sql) {
    pqxx::work txn(db());
    auto result = txn.exec(sql);
    txn.commit();
    return result;
}

int DatabaseTest::insertUser(const std::string& username) {
    pqxx::work txn(db());
    const int id = txn.query_value<int>(
        "INSERT INTO users(username, password_hash) VALUES ($1, 'not-a-hash') RETURNING id",
        pqxx::params(username));
    txn.commit();
    return id;
}
