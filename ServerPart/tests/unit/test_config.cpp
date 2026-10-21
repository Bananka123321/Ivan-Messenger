#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Config.h"
#include "TempFile.h"

using testing::HasSubstr;
using testing::Optional;

class ConfigTest : public testing::Test {
protected:
    void SetUp() override {
        Config::loaded = false;
        Config::db_config = {};
        Config::server_config = {};
    }
};

TEST_F(ConfigTest, LoadsFullConfig) {
    const TempFile file(R"({
        "database": {"name": "chat_db", "user": "ivan", "password": "pw", "host": "db.local", "port": 6543, "timeout": 3, "pool_size": 4},
        "server": {"port": 7000, "max_connections": 5}
    })");

    ASSERT_EQ(Config::load(file.path()), std::nullopt);

    const auto& db = Config::getDB();
    EXPECT_EQ(db.name, "chat_db");
    EXPECT_EQ(db.user, "ivan");
    EXPECT_EQ(db.password, "pw");
    EXPECT_EQ(db.host, "db.local");
    EXPECT_EQ(db.port, 6543);
    EXPECT_EQ(db.timeout, 3);
    EXPECT_EQ(db.pool_size, 4);

    EXPECT_EQ(Config::getServer().port, 7000);
    EXPECT_EQ(Config::getServer().max_connections, 5);
}

TEST_F(ConfigTest, AppliesDefaultsForOptionalFields) {
    const TempFile file(R"({"database": {"name": "n", "user": "u", "password": "p", "host": "h"}})");

    ASSERT_EQ(Config::load(file.path()), std::nullopt);

    EXPECT_EQ(Config::getDB().port, 5432);
    EXPECT_EQ(Config::getDB().timeout, 10);
    EXPECT_EQ(Config::getDB().pool_size, 8);
    EXPECT_EQ(Config::getServer().port, 6767);
    EXPECT_EQ(Config::getServer().max_connections, 100);
}

TEST_F(ConfigTest, AppliesServerDefaultsInsidePartialSection) {
    const TempFile file(R"({"database": {"name": "n", "user": "u", "password": "p", "host": "h"}, "server": {"port": 9000}})");

    ASSERT_EQ(Config::load(file.path()), std::nullopt);

    EXPECT_EQ(Config::getServer().port, 9000);
    EXPECT_EQ(Config::getServer().max_connections, 100);
}

TEST_F(ConfigTest, ReportsMissingFile) {
    EXPECT_THAT(Config::load("/nonexistent/dir/config.json"), Optional(HasSubstr("Config file not found")));
    EXPECT_FALSE(Config::loaded);
}

TEST_F(ConfigTest, ReportsInvalidJson) {
    const TempFile file("{ not json");
    EXPECT_THAT(Config::load(file.path()), Optional(HasSubstr("JSON parse error")));
}

TEST_F(ConfigTest, ReportsMissingDatabaseSection) {
    const TempFile file(R"({"server": {"port": 1}})");
    EXPECT_THAT(Config::load(file.path()), Optional(HasSubstr("'database'")));
}

TEST_F(ConfigTest, ReportsDatabaseSectionOfWrongType) {
    const TempFile file(R"({"database": "chat_db"})");
    EXPECT_THAT(Config::load(file.path()), Optional(HasSubstr("'database'")));
}

TEST_F(ConfigTest, ReportsMissingRequiredDatabaseField) {
    for (const std::string missing : {"name", "user", "password", "host"}) {
        nlohmann::json db = {{"name", "n"}, {"user", "u"}, {"password", "p"}, {"host", "h"}};
        db.erase(missing);
        const TempFile file(nlohmann::json({{"database", db}}).dump());

        EXPECT_THAT(Config::load(file.path()), Optional(HasSubstr("missing required fields"))) << "без поля " << missing;
    }
}

TEST_F(ConfigTest, ReportsFieldOfWrongType) {
    const TempFile file(R"({"database": {"name": "n", "user": "u", "password": "p", "host": "h", "port": "5432"}})");
    EXPECT_THAT(Config::load(file.path()), Optional(HasSubstr("JSON type error")));
}

TEST_F(ConfigTest, GettersThrowBeforeLoad) {
    EXPECT_THROW(Config::getDB(), std::runtime_error);
    EXPECT_THROW(Config::getServer(), std::runtime_error);
}

TEST(DBConfig, BuildsLibpqConnectionString) {
    const DBConfig db{.name = "chat_db", .user = "ivan", .password = "pw", .host = "localhost", .port = 5432, .timeout = 10};

    EXPECT_EQ(db.getConnectionStr(), "dbname=chat_db user=ivan password=pw host=localhost port=5432 connect_timeout=10");
}
