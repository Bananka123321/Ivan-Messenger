#include <gtest/gtest.h>

#include <QSignalSpy>

#include "LoginViewModel.h"
#include "SearchViewModel.h"

class LoginViewModelTest : public testing::Test {
protected:
    Router router_;
    Handler handler_;
    LoginViewModel vm_{&router_, &handler_, nullptr, nullptr};
};

TEST_F(LoginViewModelTest, InitiallyInvalid) {
    EXPECT_FALSE(vm_.loginValid());
    EXPECT_FALSE(vm_.passwordValid());
    EXPECT_FALSE(vm_.registerLoginValid());
    EXPECT_FALSE(vm_.passwordsMatch());
}

TEST_F(LoginViewModelTest, ValidateLoginFormTracksBothFields) {
    QSignalSpy changed(&vm_, &LoginViewModel::validationChanged);

    vm_.validateLoginForm("ivan", "123");
    EXPECT_TRUE(vm_.loginValid());
    EXPECT_FALSE(vm_.passwordValid());

    vm_.validateLoginForm("ivan", "secret1");
    EXPECT_TRUE(vm_.passwordValid());

    EXPECT_EQ(changed.count(), 2);
}

TEST_F(LoginViewModelTest, ValidationSignalOnlyOnChange) {
    QSignalSpy changed(&vm_, &LoginViewModel::validationChanged);

    vm_.validateLoginForm("ivan", "secret1");
    vm_.validateLoginForm("ivan2", "secret2");
    vm_.validateLoginForm("ivan3", "secret3");

    EXPECT_EQ(changed.count(), 1);
}

TEST_F(LoginViewModelTest, ValidateRegisterForm) {
    vm_.validateRegisterForm("ivan", "secret1", "secret2");
    EXPECT_TRUE(vm_.registerLoginValid());
    EXPECT_TRUE(vm_.registerPasswordValid());
    EXPECT_TRUE(vm_.registerPasswordRepeatValid());
    EXPECT_FALSE(vm_.passwordsMatch());

    vm_.validateRegisterForm("ivan", "secret1", "secret1");
    EXPECT_TRUE(vm_.passwordsMatch());

    vm_.validateRegisterForm("iv", "123", "123");
    EXPECT_FALSE(vm_.registerLoginValid());
    EXPECT_FALSE(vm_.registerPasswordValid());
    EXPECT_FALSE(vm_.registerPasswordRepeatValid());
    EXPECT_TRUE(vm_.passwordsMatch());
}

TEST_F(LoginViewModelTest, TryLoginRejectsInvalidInputLocally) {
    QSignalSpy failed(&vm_, &LoginViewModel::loginFailed);

    vm_.tryLogin("iv", "secret1");
    vm_.tryLogin("ivan", "123");

    ASSERT_EQ(failed.count(), 2);
    EXPECT_EQ(failed.at(0).at(0).toString(), "Must be minimum 4 characters\n");
    EXPECT_EQ(failed.at(1).at(0).toString(), "Must be minimum 6 characters\n");
}

TEST_F(LoginViewModelTest, TryRegisterChecksPasswordRepeatFirst) {
    QSignalSpy failed(&vm_, &LoginViewModel::registerFailed);

    vm_.tryRegister("iv", "secret1", "secret2");

    ASSERT_EQ(failed.count(), 1);
    EXPECT_EQ(failed.at(0).at(0).toString(), "Passwords do not match");
}

TEST_F(LoginViewModelTest, ValidInputDoesNotFailLocally) {
    QSignalSpy loginFailed(&vm_, &LoginViewModel::loginFailed);
    QSignalSpy registerFailed(&vm_, &LoginViewModel::registerFailed);

    vm_.tryLogin("ivan", "secret1");
    vm_.tryRegister("ivan", "secret1", "secret1");

    EXPECT_EQ(loginFailed.count(), 0);
    EXPECT_EQ(registerFailed.count(), 0);
}

TEST_F(LoginViewModelTest, ForwardsServerResults) {
    QSignalSpy loginOk(&vm_, &LoginViewModel::loginSuccess);
    QSignalSpy loginFailed(&vm_, &LoginViewModel::loginFailed);
    QSignalSpy registerOk(&vm_, &LoginViewModel::registerSuccess);
    QSignalSpy registerFailed(&vm_, &LoginViewModel::registerFailed);

    handler_.handleMessage(protocol::loginResponse(true, 7, "ivan", "c", "s"));
    handler_.handleMessage(protocol::loginResponse(false, -1, "", "", "", "Invalid password"));
    handler_.handleMessage(protocol::registerResponse(true, 7, "ivan", "c", "s"));
    handler_.handleMessage(protocol::registerResponse(false, -1, "", "", "", "Username is already taken"));

    EXPECT_EQ(loginOk.count(), 1);
    ASSERT_EQ(loginFailed.count(), 1);
    EXPECT_EQ(loginFailed.at(0).at(0).toString(), "Invalid password");
    EXPECT_EQ(registerOk.count(), 1);
    ASSERT_EQ(registerFailed.count(), 1);
    EXPECT_EQ(registerFailed.at(0).at(0).toString(), "Username is already taken");
}

class SearchViewModelTest : public testing::Test {
protected:
    Router router_;
    SearchViewModel vm_{&router_};
};

TEST_F(SearchViewModelTest, SearchStartsLoading) {
    QSignalSpy searching(&vm_, &SearchViewModel::isSearchingChanged);

    vm_.searchUser("iv");

    EXPECT_TRUE(vm_.isSearching());
    EXPECT_EQ(searching.count(), 1);
}

TEST_F(SearchViewModelTest, ResultsFillListAndStopLoading) {
    vm_.searchUser("iv");
    QSignalSpy users(&vm_, &SearchViewModel::usersChanged);

    vm_.onUserSearchResults({{.username = "ivan", .user_id = 1}, {.username = "ivanova", .user_id = 2}});

    EXPECT_FALSE(vm_.isSearching());
    EXPECT_EQ(users.count(), 1);
    const auto list = vm_.getUsers();
    ASSERT_EQ(list.size(), 2);
    EXPECT_EQ(list[0]["id"].toInt(), 1);
    EXPECT_EQ(list[1]["username"].toString(), "ivanova");
}

TEST_F(SearchViewModelTest, NewResultsReplaceOldOnes) {
    vm_.onUserSearchResults({{.username = "ivan", .user_id = 1}});
    vm_.onUserSearchResults({{.username = "masha", .user_id = 3}});

    ASSERT_EQ(vm_.getUsers().size(), 1);
    EXPECT_EQ(vm_.getUsers()[0]["username"].toString(), "masha");
}

TEST_F(SearchViewModelTest, BlankQueryClearsResultsWithoutLoading) {
    vm_.onUserSearchResults({{.username = "ivan", .user_id = 1}});

    vm_.searchUser("   ");

    EXPECT_TRUE(vm_.getUsers().isEmpty());
    EXPECT_FALSE(vm_.isSearching());
}

TEST_F(SearchViewModelTest, SelectUserRequestsDialog) {
    QSignalSpy selected(&vm_, &SearchViewModel::userSelectedForDialog);

    vm_.selectUser(42);

    ASSERT_EQ(selected.count(), 1);
    EXPECT_EQ(selected.at(0).at(0).toLongLong(), 42);
}
