#include <gtest/gtest.h>

#include <QSignalSpy>

#include "AppState.h"

class AppStateTest : public testing::Test {
protected:
    void SetUp() override {
        QSettings settings("IvanMessenger", "ClientConfig");
        settings.clear();
        settings.sync();
    }
};

TEST_F(AppStateTest, DefaultsWithoutSavedSettings) {
    const AppState state;

    EXPECT_EQ(state.getCurrentUserId(), -1);
    EXPECT_EQ(state.getCurrentUsername(), "");
    EXPECT_EQ(state.getSessionToken(), "");
    EXPECT_EQ(state.getConnectionToken(), "");
    EXPECT_FALSE(state.hasSession());
    EXPECT_EQ(state.getConnectionStatus(), ConnectionState::Disconnected);
    EXPECT_EQ(state.getServerHost(), "127.0.0.1");
    EXPECT_EQ(state.getServerPort(), 6767);
}

TEST_F(AppStateTest, SaveSessionUpdatesStateAndNotifies) {
    AppState state;
    QSignalSpy tokenSpy(&state, &AppState::sessionTokenChanged);
    QSignalSpy idSpy(&state, &AppState::userIdChanged);
    QSignalSpy nameSpy(&state, &AppState::usernameChanged);

    state.saveSession("token", 7, "ivan");

    EXPECT_TRUE(state.hasSession());
    EXPECT_EQ(state.getSessionToken(), "token");
    EXPECT_EQ(state.getCurrentUserId(), 7);
    EXPECT_EQ(state.getCurrentUsername(), "ivan");
    EXPECT_EQ(tokenSpy.count(), 1);
    EXPECT_EQ(idSpy.count(), 1);
    EXPECT_EQ(nameSpy.count(), 1);
}

TEST_F(AppStateTest, SavedSessionSurvivesRestart) {
    {
        AppState state;
        state.saveSession("token", 7, "ivan");
    }

    const AppState restarted;

    EXPECT_TRUE(restarted.hasSession());
    EXPECT_EQ(restarted.getSessionToken(), "token");
    EXPECT_EQ(restarted.getCurrentUserId(), 7);
    EXPECT_EQ(restarted.getCurrentUsername(), "ivan");
}

TEST_F(AppStateTest, ClearSessionRemovesSavedData) {
    {
        AppState state;
        state.saveSession("token", 7, "ivan");
        QSignalSpy tokenSpy(&state, &AppState::sessionTokenChanged);

        state.clearSession();

        EXPECT_FALSE(state.hasSession());
        EXPECT_EQ(state.getCurrentUserId(), -1);
        EXPECT_EQ(tokenSpy.count(), 1);
    }

    EXPECT_FALSE(AppState().hasSession());
}

TEST_F(AppStateTest, SessionRequiresBothTokenAndUserId) {
    AppState state;

    state.saveSession("", 7, "ivan");
    EXPECT_FALSE(state.hasSession());

    state.saveSession("token", -1, "ivan");
    EXPECT_FALSE(state.hasSession());
}

TEST_F(AppStateTest, ConnectionTokenIsNotPersisted) {
    {
        AppState state;
        state.setConnectionToken("conn");
        EXPECT_EQ(state.getConnectionToken(), "conn");
    }

    EXPECT_EQ(AppState().getConnectionToken(), "");
}

TEST_F(AppStateTest, ServerAddressIsPersistedAndNotified) {
    {
        AppState state;
        QSignalSpy spy(&state, &AppState::serverChanged);

        state.setServer("10.42.0.1", 7000);

        EXPECT_EQ(spy.count(), 1);
        EXPECT_EQ(state.getServerHost(), "10.42.0.1");
        EXPECT_EQ(state.getServerPort(), 7000);
    }

    const AppState restarted;
    EXPECT_EQ(restarted.getServerHost(), "10.42.0.1");
    EXPECT_EQ(restarted.getServerPort(), 7000);
}

TEST_F(AppStateTest, ConnectionStatusNotifiesOnlyOnChange) {
    AppState state;
    QSignalSpy spy(&state, &AppState::connectionStateChanged);

    state.setConnectionStatus(ConnectionState::Connecting);
    state.setConnectionStatus(ConnectionState::Connecting);
    state.setConnectionStatus(ConnectionState::Connected);
    state.setConnectionStatus(ConnectionState::Disconnected);

    ASSERT_EQ(spy.count(), 3);
    EXPECT_EQ(spy.at(0).at(0).value<ConnectionState>(), ConnectionState::Connecting);
    EXPECT_EQ(spy.at(1).at(0).value<ConnectionState>(), ConnectionState::Connected);
    EXPECT_EQ(spy.at(2).at(0).value<ConnectionState>(), ConnectionState::Disconnected);
}

TEST_F(AppStateTest, UsersDirectory) {
    AppState state;
    QSignalSpy spy(&state, &AppState::usersChanged);

    state.setUsers({{1, "ivan"}, {2, "masha"}});

    EXPECT_EQ(spy.count(), 1);
    EXPECT_EQ(state.getUsers().size(), 2u);
    EXPECT_EQ(state.getUsernameByUserId(2), "masha");
    EXPECT_EQ(state.getUsernameByUserId(3), "Unknown");
}

TEST_F(AppStateTest, SettersNotify) {
    AppState state;
    QSignalSpy nameSpy(&state, &AppState::usernameChanged);
    QSignalSpy idSpy(&state, &AppState::userIdChanged);
    QSignalSpy tokenSpy(&state, &AppState::sessionTokenChanged);

    state.setCurrentUsername("ivan");
    state.setCurrentUserId(5);
    state.setSessionToken("tok");

    EXPECT_EQ(state.getCurrentUsername(), "ivan");
    EXPECT_EQ(state.getCurrentUserId(), 5);
    EXPECT_EQ(state.getSessionToken(), "tok");
    EXPECT_EQ(nameSpy.count(), 1);
    EXPECT_EQ(idSpy.count(), 1);
    EXPECT_EQ(tokenSpy.count(), 1);
}
