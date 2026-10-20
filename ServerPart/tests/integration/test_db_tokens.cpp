#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <set>

#include "DB_DevicesSessionManager.h"
#include "DB_TemporaryTokenManager.h"
#include "TestDatabase.h"

class DBDevicesSessionTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        sessions_ = std::make_unique<DB_DevicesSessionManager>(TestDatabase::connectionString());
        userId_ = insertUser("ivan");
    }

    DB_DevicesSessionManager& sessions() { return *sessions_; }
    int userId() const { return userId_; }

    std::string createToken() {
        const auto token = sessions().createToken(userId_, "Pixel 7", "10.0.0.2");
        EXPECT_TRUE(token.has_value());
        return token.value_or("");
    }

    int sessionCount() { return exec("SELECT COUNT(*) FROM user_sessions")[0][0].as<int>(); }

private:
    std::unique_ptr<DB_DevicesSessionManager> sessions_;
    int userId_ = 0;
};

TEST_F(DBDevicesSessionTest, CreatesRandom64CharHexToken) {
    const auto token = createToken();

    EXPECT_THAT(token, testing::MatchesRegex("[0-9a-f]{64}"));
}

TEST_F(DBDevicesSessionTest, TokensAreUnique) {
    std::set<std::string> tokens;
    for (int i = 0; i < 20; ++i)
        tokens.insert(createToken());

    EXPECT_EQ(tokens.size(), 20u);
}

TEST_F(DBDevicesSessionTest, StoresDeviceInfo) {
    const auto token = createToken();

    const auto row = exec("SELECT user_id, device_info, ip_address FROM user_sessions WHERE token = '" + token + "'");
    ASSERT_EQ(row.size(), 1u);
    EXPECT_EQ(row[0][0].as<int>(), userId());
    EXPECT_EQ(row[0][1].as<std::string>(), "Pixel 7");
    EXPECT_EQ(row[0][2].as<std::string>(), "10.0.0.2");
}

TEST_F(DBDevicesSessionTest, NewTokenIsValidAndMapsToUser) {
    const auto token = createToken();

    EXPECT_TRUE(sessions().isValid(token));
    EXPECT_EQ(sessions().getUserIdByToken(token), userId());
}

TEST_F(DBDevicesSessionTest, UnknownTokenIsInvalid) {
    EXPECT_FALSE(sessions().isValid("deadbeef"));
    EXPECT_EQ(sessions().getUserIdByToken("deadbeef"), std::nullopt);
}

TEST_F(DBDevicesSessionTest, ExpiredTokenIsInvalidAndDeleted) {
    const auto token = createToken();
    exec("UPDATE user_sessions SET last_activity = NOW() - INTERVAL '31 days'");

    EXPECT_EQ(sessions().getUserIdByToken(token), std::nullopt);
    EXPECT_FALSE(sessions().isValid(token));
    EXPECT_EQ(sessionCount(), 0);
}

TEST_F(DBDevicesSessionTest, UpdateActivityExtendsLifetime) {
    const auto token = createToken();
    exec("UPDATE user_sessions SET last_activity = NOW() - INTERVAL '29 days'");

    sessions().updateActivity(token);

    const auto row = exec("SELECT expires_at > NOW() + INTERVAL '29 days' FROM user_sessions");
    EXPECT_TRUE(row[0][0].as<bool>());
}

TEST_F(DBDevicesSessionTest, DeleteSessionInvalidatesToken) {
    const auto token = createToken();

    sessions().deleteSession(token);

    EXPECT_FALSE(sessions().isValid(token));
}

TEST_F(DBDevicesSessionTest, DeleteAllSessionsKeepsCurrentOne) {
    const auto current = createToken();
    const auto other1 = createToken();
    const auto other2 = createToken();
    const int otherUser = insertUser("masha");
    const auto foreign = sessions().createToken(otherUser, "", "").value();

    sessions().deleteAllSessions(userId(), current);

    EXPECT_TRUE(sessions().isValid(current));
    EXPECT_FALSE(sessions().isValid(other1));
    EXPECT_FALSE(sessions().isValid(other2));
    EXPECT_TRUE(sessions().isValid(foreign));
}

TEST_F(DBDevicesSessionTest, SessionsAreRemovedWithUser) {
    createToken();

    exec("DELETE FROM users");

    EXPECT_EQ(sessionCount(), 0);
}

TEST_F(DBDevicesSessionTest, CreateTokenForUnknownUserFails) {
    EXPECT_EQ(sessions().createToken(99999, "", ""), std::nullopt);
}

class DBTemporaryTokenTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        tokens_ = std::make_unique<DB_TemporaryTokenManager>(TestDatabase::connectionString());
        userId_ = insertUser("ivan");
    }

    DB_TemporaryTokenManager& tokens() { return *tokens_; }
    int userId() const { return userId_; }

private:
    std::unique_ptr<DB_TemporaryTokenManager> tokens_;
    int userId_ = 0;
};

TEST_F(DBTemporaryTokenTest, CreatedTokenIsValidAndMapsToUser) {
    tokens().createSession(userId(), "conn-token");

    EXPECT_TRUE(tokens().isValid("conn-token"));
    EXPECT_EQ(tokens().getUserIdByToken("conn-token"), userId());
}

TEST_F(DBTemporaryTokenTest, UnknownTokenIsInvalid) {
    EXPECT_FALSE(tokens().isValid("missing"));
    EXPECT_EQ(tokens().getUserIdByToken("missing"), std::nullopt);
}

TEST_F(DBTemporaryTokenTest, DeleteSessionInvalidatesToken) {
    tokens().createSession(userId(), "conn-token");

    tokens().deleteSession("conn-token");

    EXPECT_FALSE(tokens().isValid("conn-token"));
}

TEST_F(DBTemporaryTokenTest, DuplicateTokenIsIgnored) {
    tokens().createSession(userId(), "conn-token");
    const int otherUser = insertUser("masha");

    tokens().createSession(otherUser, "conn-token");

    EXPECT_EQ(tokens().getUserIdByToken("conn-token"), userId());
}

TEST_F(DBTemporaryTokenTest, UserCanHaveSeveralTokens) {
    tokens().createSession(userId(), "phone");
    tokens().createSession(userId(), "desktop");

    EXPECT_TRUE(tokens().isValid("phone"));
    EXPECT_TRUE(tokens().isValid("desktop"));
}
