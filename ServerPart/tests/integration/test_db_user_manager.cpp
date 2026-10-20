#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "DB_UserManager.h"
#include "TestDatabase.h"

using testing::StartsWith;

class DBUserManagerTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        users_ = std::make_unique<DB_UserManager>(TestDatabase::connectionString());
    }

    DB_UserManager& users() { return *users_; }

private:
    std::unique_ptr<DB_UserManager> users_;
};

TEST_F(DBUserManagerTest, RegisterCreatesUser) {
    const auto result = users().registerUser("ivan", "secret1");

    ASSERT_TRUE(result.success) << result.error;
    EXPECT_GT(result.user_id, 0);
    EXPECT_EQ(result.error, "");
    EXPECT_EQ(users().getUsername(result.user_id), "ivan");
}

TEST_F(DBUserManagerTest, RegisterStoresArgon2HashInsteadOfPassword) {
    const auto result = users().registerUser("ivan", "secret1");
    ASSERT_TRUE(result.success);

    const auto row = exec("SELECT password_hash FROM users WHERE username = 'ivan'");
    const auto hash = row[0][0].as<std::string>();

    EXPECT_THAT(hash, StartsWith("$argon2id$"));
    EXPECT_EQ(hash.find("secret1"), std::string::npos);
}

TEST_F(DBUserManagerTest, SamePasswordGetsDifferentHashes) {
    ASSERT_TRUE(users().registerUser("ivan", "secret1").success);
    ASSERT_TRUE(users().registerUser("masha", "secret1").success);

    const auto rows = exec("SELECT password_hash FROM users ORDER BY id");
    EXPECT_NE(rows[0][0].as<std::string>(), rows[1][0].as<std::string>());
}

TEST_F(DBUserManagerTest, RegisterRejectsTakenUsername) {
    ASSERT_TRUE(users().registerUser("ivan", "secret1").success);

    const auto second = users().registerUser("ivan", "other12");

    EXPECT_FALSE(second.success);
    EXPECT_EQ(second.user_id, -1);
    EXPECT_EQ(second.error, "Username is already taken");
}

TEST_F(DBUserManagerTest, UsernameAvailability) {
    EXPECT_TRUE(users().bUsernameAvailable("ivan"));
    ASSERT_TRUE(users().registerUser("ivan", "secret1").success);
    EXPECT_FALSE(users().bUsernameAvailable("ivan"));
    EXPECT_TRUE(users().bUsernameAvailable("Ivan"));
}

TEST_F(DBUserManagerTest, LoginWithCorrectPassword) {
    const auto reg = users().registerUser("ivan", "secret1");
    ASSERT_TRUE(reg.success);

    const auto login = users().loginUser("ivan", "secret1");

    EXPECT_TRUE(login.success);
    EXPECT_EQ(login.user_id, reg.user_id);
}

TEST_F(DBUserManagerTest, LoginWithWrongPassword) {
    ASSERT_TRUE(users().registerUser("ivan", "secret1").success);

    const auto login = users().loginUser("ivan", "secret2");

    EXPECT_FALSE(login.success);
    EXPECT_EQ(login.user_id, -1);
    EXPECT_EQ(login.error, "Invalid password");
}

TEST_F(DBUserManagerTest, LoginUnknownUser) {
    const auto login = users().loginUser("ghost", "secret1");

    EXPECT_FALSE(login.success);
    EXPECT_EQ(login.error, "User not found");
}

TEST_F(DBUserManagerTest, UsernameWithQuotesIsStoredSafely) {
    const std::string tricky = "o'neil\"; DROP TABLE users; --";
    ASSERT_TRUE(users().registerUser(tricky, "secret1").success);

    EXPECT_FALSE(users().bUsernameAvailable(tricky));
    EXPECT_TRUE(users().loginUser(tricky, "secret1").success);
    EXPECT_EQ(exec("SELECT COUNT(*) FROM users")[0][0].as<int>(), 1);
}

TEST_F(DBUserManagerTest, SearchFindsUsersByPrefix) {
    insertUser("ivan");
    insertUser("ivanova");
    insertUser("petr");

    const auto found = users().searchUsers("iva");

    std::vector<std::string> names;
    for (const auto& u : found)
        names.push_back(u.username);
    EXPECT_THAT(names, testing::UnorderedElementsAre("ivan", "ivanova"));
}

TEST_F(DBUserManagerTest, SearchReturnsIdsMatchingUsernames) {
    const int id = insertUser("masha");

    const auto found = users().searchUsers("masha");

    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0].user_id, id);
}

TEST_F(DBUserManagerTest, SearchIsCaseSensitiveAndPrefixOnly) {
    insertUser("ivan");

    EXPECT_TRUE(users().searchUsers("IVAN").empty());
    EXPECT_TRUE(users().searchUsers("van").empty());
}

TEST_F(DBUserManagerTest, SearchReturnsAtMost20Users) {
    for (int i = 0; i < 25; ++i)
        insertUser("user" + std::to_string(i));

    EXPECT_EQ(users().searchUsers("user").size(), 20u);
}

TEST_F(DBUserManagerTest, GetUsernameOfUnknownUser) {
    EXPECT_EQ(users().getUsername(12345), std::nullopt);
}
