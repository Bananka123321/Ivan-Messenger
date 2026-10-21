#include <gtest/gtest.h>

#include <QSignalSpy>

#include "Handler.h"

namespace {

constexpr int64_t kTimestampMs = 1760968800123;

Message text(const int64_t id, const std::string& body) {
    Message m;
    m.id = id;
    m.dialog_id = 15;
    m.sender_id = 3;
    m.created_at_ms = kTimestampMs;
    m.payload = TextContent{.text = body};
    return m;
}

template <typename T>
T arg(const QSignalSpy& spy, const int index) {
    return spy.at(0).at(index).value<T>();
}

}

class ClientHandlerTest : public testing::Test {
protected:
    void receive(const std::string& packet) { handler_.handleMessage(packet); }

    Handler handler_;
};

TEST_F(ClientHandlerTest, LoginSuccess) {
    QSignalSpy ok(&handler_, &Handler::S_loginSuccess);
    QSignalSpy fail(&handler_, &Handler::S_loginFailed);

    receive(protocol::loginResponse(true, 7, "ivan", "conn", "sess"));

    ASSERT_EQ(ok.count(), 1);
    EXPECT_EQ(fail.count(), 0);
    EXPECT_EQ(arg<std::string>(ok, 0), "ivan");
    EXPECT_EQ(arg<int>(ok, 1), 7);
    EXPECT_EQ(arg<std::string>(ok, 2), "conn");
    EXPECT_EQ(arg<std::string>(ok, 3), "sess");
}

TEST_F(ClientHandlerTest, LoginFailureCarriesReason) {
    QSignalSpy ok(&handler_, &Handler::S_loginSuccess);
    QSignalSpy fail(&handler_, &Handler::S_loginFailed);

    receive(protocol::loginResponse(false, -1, "", "", "", "Invalid password"));

    EXPECT_EQ(ok.count(), 0);
    ASSERT_EQ(fail.count(), 1);
    EXPECT_EQ(arg<std::string>(fail, 0), "Invalid password");
}

TEST_F(ClientHandlerTest, RegisterSuccessAndFailure) {
    QSignalSpy ok(&handler_, &Handler::S_registerSuccess);
    QSignalSpy fail(&handler_, &Handler::S_registerFailed);

    receive(protocol::registerResponse(true, 8, "masha", "c", "s"));
    receive(protocol::registerResponse(false, -1, "", "", "", "Username is already taken"));

    ASSERT_EQ(ok.count(), 1);
    EXPECT_EQ(arg<int>(ok, 1), 8);
    ASSERT_EQ(fail.count(), 1);
    EXPECT_EQ(arg<std::string>(fail, 0), "Username is already taken");
}

TEST_F(ClientHandlerTest, IncomingMessage) {
    QSignalSpy spy(&handler_, &Handler::S_Message);

    receive(protocol::sendMessage(text(5'000'000'000, "Привет")));

    ASSERT_EQ(spy.count(), 1);
    const auto msg = arg<Message>(spy, 0);
    EXPECT_EQ(msg.id, 5'000'000'000);
    EXPECT_EQ(msg.dialog_id, 15);
    EXPECT_EQ(msg.sender_id, 3);
    EXPECT_EQ(msg.created_at_ms, kTimestampMs);
    EXPECT_EQ(std::get<TextContent>(msg.payload).text, "Привет");
}

TEST_F(ClientHandlerTest, SearchResults) {
    QSignalSpy spy(&handler_, &Handler::S_UserSearch);

    receive(protocol::searchUserResponse({{.username = "ivan", .user_id = 1}, {.username = "ivanova", .user_id = 2}}));

    ASSERT_EQ(spy.count(), 1);
    const auto users = arg<std::vector<User>>(spy, 0);
    ASSERT_EQ(users.size(), 2u);
    EXPECT_EQ(users[1].username, "ivanova");
    EXPECT_EQ(users[1].user_id, 2);
}

TEST_F(ClientHandlerTest, HistoryLoadedForDialog) {
    QSignalSpy spy(&handler_, &Handler::S_HistoryLoaded);

    receive(protocol::historyResponse(true, 15, {text(2, "b"), text(1, "a")}));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(arg<int64_t>(spy, 0), 15);
    const auto messages = arg<std::vector<Message>>(spy, 1);
    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0].id, 2);
}

TEST_F(ClientHandlerTest, FailedHistoryEmitsNothing) {
    QSignalSpy spy(&handler_, &Handler::S_HistoryLoaded);

    receive(protocol::historyResponse(false, 15, {}, "boom"));

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ClientHandlerTest, DialogsLoaded) {
    QSignalSpy spy(&handler_, &Handler::S_DialogsLoaded);
    MetaDialog_Client md;
    md.dialog_id = 15;
    md.peer_id = 3;
    md.username = "masha";
    md.last_msg_preview = "hi";
    md.last_msg_timestamp = kTimestampMs;

    receive(protocol::getDialogsResponse(true, {md}));

    ASSERT_EQ(spy.count(), 1);
    const auto dialogs = arg<std::vector<MetaDialog_Client>>(spy, 0);
    ASSERT_EQ(dialogs.size(), 1u);
    EXPECT_EQ(dialogs[0].dialog_id, 15);
    EXPECT_EQ(dialogs[0].username, "masha");
    EXPECT_EQ(dialogs[0].last_msg_timestamp, kTimestampMs);
}

TEST_F(ClientHandlerTest, EmptyDialogListIsReportedAsFailureByServer) {
    QSignalSpy spy(&handler_, &Handler::S_DialogsLoaded);

    receive(protocol::getDialogsResponse(false, {}));

    EXPECT_EQ(spy.count(), 0);
}

TEST_F(ClientHandlerTest, DialogOpened) {
    QSignalSpy spy(&handler_, &Handler::S_DialogOpened);

    receive(protocol::openDialogResponse(true, 15, {.username = "masha", .user_id = 3}, {text(1, "a")}));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_TRUE(arg<bool>(spy, 0));
    EXPECT_EQ(arg<int64_t>(spy, 1), 15);
    EXPECT_EQ(arg<User>(spy, 2).username, "masha");
    EXPECT_EQ(arg<std::vector<Message>>(spy, 3).size(), 1u);
    EXPECT_EQ(arg<std::string>(spy, 4), "");
}

TEST_F(ClientHandlerTest, DialogOpenFailed) {
    QSignalSpy spy(&handler_, &Handler::S_DialogOpened);

    receive(protocol::openDialogResponse(false, 0, User{}, {}, "User not found"));

    ASSERT_EQ(spy.count(), 1);
    EXPECT_FALSE(arg<bool>(spy, 0));
    EXPECT_EQ(arg<std::string>(spy, 4), "User not found");
}

TEST_F(ClientHandlerTest, ResumeSessionReportsBothOutcomes) {
    QSignalSpy spy(&handler_, &Handler::S_ResumeSessionResponse);

    receive(protocol::resumeSessionResponse(true, "new-conn"));
    receive(protocol::resumeSessionResponse(false));

    ASSERT_EQ(spy.count(), 2);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
    EXPECT_EQ(spy.at(0).at(1).value<std::string>(), "new-conn");
    EXPECT_FALSE(spy.at(1).at(0).toBool());
}

TEST_F(ClientHandlerTest, ResumeConnectionSignalsOnlySuccess) {
    QSignalSpy spy(&handler_, &Handler::S_ResumeConnectionSuccess);

    receive(protocol::resumeConnectionResponse(false));
    EXPECT_EQ(spy.count(), 0);

    receive(protocol::resumeConnectionResponse(true));
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(ClientHandlerTest, ServerErrorDoesNotEmitBusinessSignals) {
    QSignalSpy login(&handler_, &Handler::S_loginFailed);
    QSignalSpy message(&handler_, &Handler::S_Message);

    receive(protocol::errorMessage("Not authenticated"));

    EXPECT_EQ(login.count(), 0);
    EXPECT_EQ(message.count(), 0);
}

TEST_F(ClientHandlerTest, MalformedPacketsAreIgnored) {
    QSignalSpy message(&handler_, &Handler::S_Message);

    for (const std::string packet : {"", "garbage", "{", "[]", R"({"type": "unknown"})",
                                     R"({"type": "sendMessage"})",
                                     R"({"type": "sendMessage", "data": {"type": 99, "payload": {}}})",
                                     R"({"type": "loginResponse", "success": "yes"})"})
        receive(packet);

    EXPECT_EQ(message.count(), 0);
}
