#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QTest>

#include "Appcontroller.h"
#include "FakeServer.h"

using nlohmann::json;

namespace {

constexpr int64_t kTimestampMs = 1760968800123;

Message incoming(const int64_t id, const int64_t dialogId, const std::string& body) {
    Message m;
    m.id = id;
    m.dialog_id = static_cast<int>(dialogId);
    m.sender_id = 3;
    m.created_at_ms = kTimestampMs;
    m.payload = TextContent{.text = body};
    return m;
}

MetaDialog_Client dialog(const int64_t id, const int peerId, const std::string& name) {
    MetaDialog_Client md;
    md.dialog_id = id;
    md.peer_id = peerId;
    md.username = name;
    md.last_msg_preview = "hi";
    md.last_msg_timestamp = kTimestampMs;
    return md;
}

}

class AppFlowTest : public testing::Test {
protected:
    void SetUp() override {
        QSettings settings("IvanMessenger", "ClientConfig");
        settings.clear();
        settings.sync();

        state_ = std::make_unique<AppState>();
        client_ = std::make_unique<TCPClient>("127.0.0.1", server_.port(), &router_);
        client_->setTrustedCertificates({FakeServer::certificate()});
        dialogs_ = std::make_unique<DialogManager>(&handler_, state_.get());
        controller_ = std::make_unique<AppController>(&router_, state_.get(), &handler_, client_.get(), dialogs_.get());

        client_->onMessage = [this](const std::string& packet) { handler_.handleMessage(packet); };
        dialogs_->start();
    }

    void connect() {
        client_->start();
        ASSERT_TRUE(server_.waitForClient());
        ASSERT_TRUE(QTest::qWaitFor([&] { return state_->getConnectionStatus() == ConnectionState::Connected; }, 5000));
    }

    void login() {
        controller_->loginRequest("ivan", "secret1");
        ASSERT_TRUE(server_.waitForPacket("loginRequest"));
        server_.send(protocol::loginResponse(true, 7, "ivan", "conn-1", "sess-1"));
        ASSERT_TRUE(server_.waitForPacket("getDialogsRequest"));
    }

    FakeServer server_;
    Router router_;
    Handler handler_;
    std::unique_ptr<AppState> state_;
    std::unique_ptr<TCPClient> client_;
    std::unique_ptr<DialogManager> dialogs_;
    std::unique_ptr<AppController> controller_;
};

TEST_F(AppFlowTest, ConnectionStatusFollowsSocket) {
    QSignalSpy spy(state_.get(), &AppState::connectionStateChanged);

    connect();
    server_.dropClient();
    ASSERT_TRUE(QTest::qWaitFor([&] { return state_->getConnectionStatus() == ConnectionState::Disconnected; }, 5000));

    ASSERT_GE(spy.count(), 3);
    EXPECT_EQ(spy.at(0).at(0).value<ConnectionState>(), ConnectionState::Connecting);
    EXPECT_EQ(spy.at(1).at(0).value<ConnectionState>(), ConnectionState::Connected);
    EXPECT_EQ(spy.at(2).at(0).value<ConnectionState>(), ConnectionState::Disconnected);
}

TEST_F(AppFlowTest, LoginSavesSessionAndLoadsDialogs) {
    connect();

    login();
    server_.send(protocol::getDialogsResponse(true, {dialog(15, 3, "masha")}));

    EXPECT_TRUE(state_->hasSession());
    EXPECT_EQ(state_->getCurrentUserId(), 7);
    EXPECT_EQ(state_->getCurrentUsername(), "ivan");
    EXPECT_EQ(state_->getSessionToken(), "sess-1");
    EXPECT_EQ(state_->getConnectionToken(), "conn-1");
    ASSERT_TRUE(QTest::qWaitFor([&] { return dialogs_->getDialogs().size() == 1; }, 5000));
    EXPECT_EQ(dialogs_->getDialogs()[0].username, "masha");
}

TEST_F(AppFlowTest, SavedSessionIsResumedAfterConnecting) {
    state_->saveSession("sess-1", 7, "ivan");
    QSignalSpy finished(controller_.get(), &AppController::resumeSessionFinished);

    controller_->checkAndResumeSession();
    connect();

    const auto request = server_.waitForPacket("resumeSessionRequest");
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ((*request)["token"], "sess-1");

    server_.send(protocol::resumeSessionResponse(true, "conn-2"));

    ASSERT_TRUE(QTest::qWaitFor([&] { return finished.count() == 1; }, 5000));
    EXPECT_TRUE(finished.at(0).at(0).toBool());
    EXPECT_EQ(state_->getConnectionToken(), "conn-2");
    EXPECT_TRUE(server_.waitForPacket("getDialogsRequest").has_value());
}

TEST_F(AppFlowTest, RejectedSessionIsReported) {
    state_->saveSession("expired", 7, "ivan");
    QSignalSpy finished(controller_.get(), &AppController::resumeSessionFinished);
    controller_->checkAndResumeSession();
    connect();
    ASSERT_TRUE(server_.waitForPacket("resumeSessionRequest"));

    server_.send(protocol::resumeSessionResponse(false));

    ASSERT_TRUE(QTest::qWaitFor([&] { return finished.count() == 1; }, 5000));
    EXPECT_FALSE(finished.at(0).at(0).toBool());
}

TEST_F(AppFlowTest, NoSavedSessionFinishesImmediately) {
    QSignalSpy finished(controller_.get(), &AppController::resumeSessionFinished);

    controller_->checkAndResumeSession();

    ASSERT_EQ(finished.count(), 1);
    EXPECT_FALSE(finished.at(0).at(0).toBool());
}

TEST_F(AppFlowTest, ReconnectRestoresConnectionWithToken) {
    connect();
    login();

    server_.dropClient();
    ASSERT_TRUE(server_.waitForClient(5000));

    const auto request = server_.waitForPacket("resumeConnectionRequest");
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ((*request)["token"], "conn-1");
}

TEST_F(AppFlowTest, MessageFromUnknownDialogRefreshesDialogList) {
    connect();
    login();

    server_.send(protocol::sendMessage(incoming(100, 42, "ты тут?")));

    ASSERT_TRUE(server_.waitForPacket("getDialogsRequest").has_value());
    const auto* messages = dialogs_->getMessages(42);
    ASSERT_NE(messages, nullptr);
    ASSERT_EQ(messages->size(), 1u);
    EXPECT_EQ(std::get<TextContent>(messages->front().payload).text, "ты тут?");
}

TEST_F(AppFlowTest, OpenDialogShowsHistory) {
    connect();
    login();
    QSignalSpy opened(controller_.get(), &AppController::dialogOpenedSuccessfully);

    controller_->openDialog(3);
    const auto request = server_.waitForPacket("openDialogRequest");
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ((*request)["target_user_id"], 3);

    server_.send(protocol::openDialogResponse(true, 15, {.username = "masha", .user_id = 3}, {incoming(1, 15, "старое")}));

    ASSERT_TRUE(QTest::qWaitFor([&] { return opened.count() == 1; }, 5000));
    EXPECT_EQ(opened.at(0).at(0).toLongLong(), 15);
    EXPECT_EQ(opened.at(0).at(1).toString(), "masha");
    EXPECT_EQ(dialogs_->getMessages(15)->size(), 1u);
}

TEST_F(AppFlowTest, OpenDialogFailureIsReported) {
    connect();
    login();
    QSignalSpy failed(controller_.get(), &AppController::dialogOpenFailed);

    controller_->openDialog(99);
    ASSERT_TRUE(server_.waitForPacket("openDialogRequest"));
    server_.send(protocol::openDialogResponse(false, 0, User{}, {}, "User not found"));

    ASSERT_TRUE(QTest::qWaitFor([&] { return failed.count() == 1; }, 5000));
    EXPECT_EQ(failed.at(0).at(0).toString(), "User not found");
}

TEST_F(AppFlowTest, SendTextGoesToServerAndAppearsLocally) {
    connect();
    login();

    controller_->sendText(15, "  Привет!  ");

    const auto request = server_.waitForPacket("sendMessage");
    ASSERT_TRUE(request.has_value());
    const auto msg = (*request)["data"].get<Message>();
    EXPECT_EQ(msg.dialog_id, 15);
    EXPECT_EQ(msg.sender_id, 7);
    EXPECT_EQ(msg.type, MessageType::Text);
    EXPECT_EQ(std::get<TextContent>(msg.payload).text, "Привет!");

    ASSERT_NE(dialogs_->getMessages(15), nullptr);
    EXPECT_EQ(dialogs_->getMessages(15)->size(), 1u);
}

TEST_F(AppFlowTest, BlankTextIsNotSent) {
    connect();
    login();

    controller_->sendText(15, "   ");
    controller_->openDialog(3);

    EXPECT_EQ(server_.takeTypes(1), std::vector<std::string>{"openDialogRequest"});
}
