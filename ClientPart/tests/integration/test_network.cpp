#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QTest>

#include "FakeServer.h"
#include "TcpClient.h"

using nlohmann::json;

class NetworkTest : public testing::Test {
protected:
    void SetUp() override {
        client_.onMessage = [this](const std::string& packet) { received_.push_back(json::parse(packet)); };
    }

    void connect() {
        QSignalSpy connected(&client_, &TCPClient::connected);
        client_.start();
        ASSERT_TRUE(server_.waitForClient());
        ASSERT_TRUE(QTest::qWaitFor([&] { return connected.count() == 1; }, 5000));
    }

    FakeServer server_;
    Router router_;
    TCPClient client_{"127.0.0.1", server_.port(), &router_};
    std::vector<json> received_;
};

TEST_F(NetworkTest, ConnectsOverTls) {
    QSignalSpy connecting(&client_, &TCPClient::connecting);

    connect();

    EXPECT_EQ(connecting.count(), 1);
    EXPECT_TRUE(client_.isConnected());
}

TEST_F(NetworkTest, RouterSendsFramedRequests) {
    connect();

    router_.loginRequest("ivan", "secret1");
    router_.searchUser("mas");

    EXPECT_EQ(server_.waitForPacket("loginRequest"), json::parse(protocol::loginRequest("ivan", "secret1")));
    EXPECT_EQ(server_.waitForPacket("searchUserRequest"), json::parse(protocol::searchUserRequest("mas")));
}

TEST_F(NetworkTest, HistoryRequestFitsServerLimit) {
    connect();

    router_.historyRequest(15, 100);

    const auto request = server_.waitForPacket("historyRequest");
    ASSERT_TRUE(request.has_value());
    EXPECT_EQ((*request)["dialog_id"], 15);
    EXPECT_EQ((*request)["last_msg_id"], 100);
    EXPECT_GE((*request)["limit"].get<int>(), 1);
    EXPECT_LE((*request)["limit"].get<int>(), protocol::HISTORY_MAX_LIMIT);
}

TEST_F(NetworkTest, WhileReconnectingOnlyResumeRequestsAreSent) {
    connect();
    router_.setReconnecting(true);

    router_.ping();
    router_.getDialogsRequest();
    router_.resumeSessionRequest("sess");
    router_.resumeConnectionRequest("conn");

    EXPECT_EQ(server_.takeTypes(2), (std::vector<std::string>{"resumeSessionRequest", "resumeConnectionRequest"}));
}

TEST_F(NetworkTest, RequestsWithoutConnectionAreDropped) {
    router_.ping();
    router_.loginRequest("ivan", "secret1");

    connect();
    router_.ping();

    EXPECT_EQ(server_.takeTypes(1), std::vector<std::string>{"ping"});
}

TEST_F(NetworkTest, ReceivesCoalescedAndSplitPackets) {
    connect();
    const QByteArray a = FakeServer::frame(protocol::ping());
    const QByteArray b = FakeServer::frame(protocol::errorMessage("x"));
    const QByteArray c = FakeServer::frame(protocol::resumeConnectionResponse(true));

    server_.sendRaw(a + b);
    server_.sendRaw(c.left(3));
    QTest::qWait(50);
    server_.sendRaw(c.mid(3));

    ASSERT_TRUE(QTest::qWaitFor([&] { return received_.size() == 3; }, 5000));
    EXPECT_EQ(received_[0]["type"], "ping");
    EXPECT_EQ(received_[1]["type"], "error");
    EXPECT_EQ(received_[2]["type"], "resumeConnectionResponse");
}

TEST_F(NetworkTest, ServerDisconnectIsReportedAndClientReconnects) {
    connect();
    QSignalSpy lost(&client_, &TCPClient::connectionLose);
    QSignalSpy connected(&client_, &TCPClient::connected);

    server_.dropClient();

    ASSERT_TRUE(QTest::qWaitFor([&] { return lost.count() >= 1; }, 5000));
    EXPECT_FALSE(client_.isConnected());

    ASSERT_TRUE(server_.waitForClient(5000));
    EXPECT_TRUE(QTest::qWaitFor([&] { return connected.count() == 1; }, 5000));
}

TEST_F(NetworkTest, SetServerSwitchesToAnotherServer) {
    connect();
    FakeServer other;

    client_.setServer("127.0.0.1", other.port());

    ASSERT_TRUE(other.waitForClient());
    router_.ping();
    EXPECT_TRUE(other.waitForPacket("ping").has_value());
}
