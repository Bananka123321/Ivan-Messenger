#pragma once

#include <gtest/gtest.h>

#include "Handler.h"
#include "TestDatabase.h"
#include "TlsPair.h"

struct TestClient {
    TlsPair tls;
    std::shared_ptr<ClientSession> session;

    TestClient() : session(std::make_shared<ClientSession>(-1, tls.takeServer())) {}

    std::vector<nlohmann::json> receive() {
        std::vector<nlohmann::json> result;
        for (const auto& packet : tls.clientReadPackets())
            result.push_back(nlohmann::json::parse(packet));
        return result;
    }

    nlohmann::json receiveOne() {
        auto packets = receive();
        EXPECT_EQ(packets.size(), 1u) << "ожидался один ответ";
        return packets.empty() ? nlohmann::json() : packets.front();
    }
};

class HandlerTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;

        config_ = TestDatabase::makeServerConfig();
        ASSERT_EQ(Config::load(config_->path()), std::nullopt);
        handler_ = std::make_unique<Handler>(sessions_, pool());
    }

    void send(TestClient& client, const std::string& packet) {
        handler_->handleMessage(client.session, packet);
    }

    nlohmann::json registerUser(TestClient& client, const std::string& username, const std::string& password = "secret1") {
        send(client, protocol::registerRequest(username, password));
        return client.receiveOne();
    }

    int signUp(TestClient& client, const std::string& username) {
        const auto response = registerUser(client, username);
        EXPECT_TRUE(response.value("success", false)) << response.dump();
        return response.value("user_id", -1);
    }

    int64_t openDialog(TestClient& client, const int targetUserId) {
        send(client, protocol::openDialogRequest(targetUserId));
        const auto response = client.receiveOne();
        EXPECT_TRUE(response.value("success", false)) << response.dump();
        return response.value("dialog_id", int64_t{0});
    }

    static std::string textMessage(const int64_t dialogId, const int senderId, const std::string& body) {
        Message m;
        m.dialog_id = static_cast<int>(dialogId);
        m.sender_id = senderId;
        m.type = MessageType::Text;
        m.payload = TextContent{.text = body};
        return protocol::sendMessage(m);
    }

    SessionManager sessions_;
    std::unique_ptr<Handler> handler_;

private:
    std::unique_ptr<TempFile> config_;
};
