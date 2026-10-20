#pragma once

#include <gtest/gtest.h>
#include <pqxx/pqxx>
#include <memory>
#include <string>

#include "TempFile.h"

namespace TestDatabase {
    struct Settings {
        std::string host;
        int port;
        std::string name;
        std::string user;
        std::string password;
    };

    const Settings& settings();
    std::string connectionString();

    std::unique_ptr<TempFile> makeServerConfig();

    void ensureSchema();

    void truncateAll();
}

class DatabaseTest : public testing::Test {
protected:
    static void SetUpTestSuite();
    void SetUp() override;

    pqxx::connection& db();

    pqxx::result exec(const std::string& sql);

    int insertUser(const std::string& username);

private:
    static inline std::string unavailableReason_;
    std::unique_ptr<pqxx::connection> db_;
};
