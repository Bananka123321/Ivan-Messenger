#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "HandlerTest.h"

using nlohmann::json;
using testing::MatchesRegex;

class HandlerAuthTest : public HandlerTest {};

TEST_F(HandlerAuthTest, RegisterReturnsTokensAndAuthenticatesSession) {
    TestClient client;

    const auto response = registerUser(client, "ivan");

    EXPECT_EQ(response["type"], "registerResponse");
    EXPECT_EQ(response["success"], true);
    EXPECT_GT(response["user_id"].get<int>(), 0);
    EXPECT_EQ(response["username"], "ivan");
    EXPECT_THAT(response["connectToken"].get<std::string>(), MatchesRegex("[0-9a-f]{32}"));
    EXPECT_THAT(response["sessionToken"].get<std::string>(), MatchesRegex("[0-9a-f]{64}"));
    EXPECT_EQ(response["error"], "");

    EXPECT_TRUE(client.session->getIsAuthenticated());
    EXPECT_EQ(client.session->getUserId(), response["user_id"].get<int>());
    EXPECT_EQ(client.session->getUsername(), "ivan");
    EXPECT_EQ(sessions_.getByUserId(response["user_id"]), client.session);
}

TEST_F(HandlerAuthTest, RegisterPersistsTokens) {
    TestClient client;
    const auto response = registerUser(client, "ivan");

    const auto connect = exec("SELECT user_id FROM connect_tokens WHERE token = '" + response["connectToken"].get<std::string>() + "'");
    const auto session = exec("SELECT user_id FROM user_sessions WHERE token = '" + response["sessionToken"].get<std::string>() + "'");

    ASSERT_EQ(connect.size(), 1u);
    ASSERT_EQ(session.size(), 1u);
    EXPECT_EQ(connect[0][0].as<int>(), response["user_id"].get<int>());
    EXPECT_EQ(session[0][0].as<int>(), response["user_id"].get<int>());
}

TEST_F(HandlerAuthTest, RegisterRejectsInvalidUsername) {
    TestClient client;

    const auto response = registerUser(client, "ab");

    EXPECT_EQ(response["success"], false);
    EXPECT_EQ(response["error"], "Must be minimum 4 characters\n");
    EXPECT_FALSE(client.session->getIsAuthenticated());
}

TEST_F(HandlerAuthTest, RegisterRejectsShortPassword) {
    TestClient client;

    const auto response = registerUser(client, "ivan", "123");

    EXPECT_EQ(response["success"], false);
    EXPECT_EQ(response["error"], "Must be minimum 6 characters\n");
}

TEST_F(HandlerAuthTest, RegisterRejectsTakenUsernameWithReason) {
    TestClient first;
    TestClient second;
    signUp(first, "ivan");

    const auto response = registerUser(second, "ivan");

    EXPECT_EQ(response["success"], false);
    EXPECT_EQ(response["error"], "Username is already taken");
    EXPECT_EQ(response["sessionToken"], "");
    EXPECT_FALSE(second.session->getIsAuthenticated());
}

TEST_F(HandlerAuthTest, LoginWithCorrectPassword) {
    TestClient registration;
    const int id = signUp(registration, "ivan");
    TestClient client;

    send(client, protocol::loginRequest("ivan", "secret1"));
    const auto response = client.receiveOne();

    EXPECT_EQ(response["type"], "loginResponse");
    EXPECT_EQ(response["success"], true);
    EXPECT_EQ(response["user_id"], id);
    EXPECT_THAT(response["sessionToken"].get<std::string>(), MatchesRegex("[0-9a-f]{64}"));
    EXPECT_TRUE(client.session->getIsAuthenticated());
}

TEST_F(HandlerAuthTest, LoginWithWrongPasswordReportsReason) {
    TestClient registration;
    signUp(registration, "ivan");
    TestClient client;

    send(client, protocol::loginRequest("ivan", "wrong-password"));
    const auto response = client.receiveOne();

    EXPECT_EQ(response["success"], false);
    EXPECT_EQ(response["error"], "Invalid password");
    EXPECT_EQ(response["sessionToken"], "");
    EXPECT_FALSE(client.session->getIsAuthenticated());
}

TEST_F(HandlerAuthTest, LoginUnknownUserReportsReason) {
    TestClient client;

    send(client, protocol::loginRequest("ghost", "secret1"));
    const auto response = client.receiveOne();

    EXPECT_EQ(response["success"], false);
    EXPECT_EQ(response["error"], "User not found");
}

TEST_F(HandlerAuthTest, LoginWithoutPasswordField) {
    TestClient client;

    send(client, R"({"type": "loginRequest", "username": "ivan"})");
    const auto response = client.receiveOne();

    EXPECT_EQ(response["success"], false);
    EXPECT_EQ(response["error"], "Invalid key");
}

TEST_F(HandlerAuthTest, EachLoginCreatesSeparateDeviceSession) {
    TestClient registration;
    signUp(registration, "ivan");
    TestClient phone;
    TestClient desktop;

    send(phone, protocol::loginRequest("ivan", "secret1"));
    send(desktop, protocol::loginRequest("ivan", "secret1"));

    EXPECT_NE(phone.receiveOne()["sessionToken"], desktop.receiveOne()["sessionToken"]);
    EXPECT_EQ(exec("SELECT COUNT(*) FROM user_sessions")[0][0].as<int>(), 3);
}

TEST_F(HandlerAuthTest, ResumeSessionWithValidToken) {
    TestClient registration;
    const auto reg = registerUser(registration, "ivan");
    TestClient client;

    send(client, protocol::resumeSessionRequest(reg["sessionToken"]));
    const auto response = client.receiveOne();

    EXPECT_EQ(response["type"], "resumeSessionResponse");
    EXPECT_EQ(response["success"], true);
    EXPECT_THAT(response["token"].get<std::string>(), MatchesRegex("[0-9a-f]{32}"));
    EXPECT_NE(response["token"], reg["connectToken"]);
    EXPECT_TRUE(client.session->getIsAuthenticated());
    EXPECT_EQ(client.session->getUserId(), reg["user_id"].get<int>());
}

TEST_F(HandlerAuthTest, ResumeSessionWithUnknownOrEmptyToken) {
    for (const std::string token : {"", "deadbeef"}) {
        TestClient client;

        send(client, protocol::resumeSessionRequest(token));
        const auto response = client.receiveOne();

        EXPECT_EQ(response["success"], false) << "token='" << token << "'";
        EXPECT_FALSE(client.session->getIsAuthenticated());
    }
}

TEST_F(HandlerAuthTest, ResumeSessionWithExpiredToken) {
    TestClient registration;
    const auto reg = registerUser(registration, "ivan");
    exec("UPDATE user_sessions SET last_activity = NOW() - INTERVAL '31 days'");
    TestClient client;

    send(client, protocol::resumeSessionRequest(reg["sessionToken"]));

    EXPECT_EQ(client.receiveOne()["success"], false);
}

TEST_F(HandlerAuthTest, ResumeConnectionWithConnectToken) {
    TestClient registration;
    const auto reg = registerUser(registration, "ivan");
    TestClient client;

    send(client, protocol::resumeConnectionRequest(reg["connectToken"]));
    const auto response = client.receiveOne();

    EXPECT_EQ(response, json({{"type", "resumeConnectionResponse"}, {"success", true}}));
    EXPECT_TRUE(client.session->getIsAuthenticated());
    EXPECT_EQ(client.session->getUsername(), "ivan");
}

TEST_F(HandlerAuthTest, ResumeConnectionRejectsSessionTokenAndGarbage) {
    TestClient registration;
    const auto reg = registerUser(registration, "ivan");

    for (const std::string token : {std::string(), std::string("garbage"), reg["sessionToken"].get<std::string>()}) {
        TestClient client;
        send(client, protocol::resumeConnectionRequest(token));
        EXPECT_EQ(client.receiveOne()["success"], false) << "token='" << token << "'";
    }
}

TEST_F(HandlerAuthTest, ProtectedRequestsRequireAuthentication) {
    TestClient client;
    const std::vector<std::string> requests = {
        textMessage(1, 1, "hi"),
        protocol::searchUserRequest("ivan"),
        protocol::historyRequest(1, 100, 10),
        protocol::getDialogsRequest(),
    };

    for (const auto& request : requests) {
        send(client, request);
        EXPECT_EQ(client.receiveOne(), json({{"type", "error"}, {"message", "Not authenticated"}})) << request;
    }
}

TEST_F(HandlerAuthTest, OpenDialogRequiresAuthentication) {
    TestClient client;

    send(client, protocol::openDialogRequest(1));
    const auto response = client.receiveOne();

    EXPECT_EQ(response["type"], "openDialogResponse");
    EXPECT_EQ(response["success"], false);
    EXPECT_EQ(response["error"], "Not authenticated");
}

TEST_F(HandlerAuthTest, MalformedPacketsAreIgnored) {
    TestClient client;

    for (const std::string packet : {"", "not json", "{", "[]", R"({"type": 5})", R"({"type": "unknownRequest"})", R"({"no_type": true})"})
        send(client, packet);

    EXPECT_TRUE(client.receive().empty());
}

TEST_F(HandlerAuthTest, WrongFieldTypesDoNotCrashServer) {
    TestClient client;

    send(client, R"({"type": "loginRequest", "username": 123, "password": []})");
    send(client, R"({"type": "registerRequest", "username": null, "password": {}})");

    EXPECT_TRUE(client.receive().empty());
    EXPECT_EQ(registerUser(client, "ivan")["success"], true);
}

TEST_F(HandlerAuthTest, PingUpdatesActivityWithoutResponse) {
    TestClient client;
    client.session->setLastActivity(0);

    send(client, protocol::ping());

    EXPECT_TRUE(client.receive().empty());
    EXPECT_GT(client.session->getLastActivity(), 0);
}
