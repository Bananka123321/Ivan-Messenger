#include <gtest/gtest.h>

#include "protocol.h"

using nlohmann::json;

namespace {

json parse(const std::string& packet) {
    return json::parse(packet);
}

}

TEST(ProtocolRequests, LoginAndRegister) {
    const auto login = parse(protocol::loginRequest("ivan", "secret1"));
    EXPECT_EQ(login, json({{"type", "loginRequest"}, {"username", "ivan"}, {"password", "secret1"}}));

    const auto reg = parse(protocol::registerRequest("ivan", "secret1"));
    EXPECT_EQ(reg, json({{"type", "registerRequest"}, {"username", "ivan"}, {"password", "secret1"}}));
}

TEST(ProtocolRequests, SearchHistoryDialogs) {
    EXPECT_EQ(parse(protocol::searchUserRequest("iv")), json({{"type", "searchUserRequest"}, {"username", "iv"}}));

    EXPECT_EQ(parse(protocol::historyRequest(5'000'000'000, 77, 50)),
              json({{"type", "historyRequest"}, {"dialog_id", 5'000'000'000}, {"last_msg_id", 77}, {"limit", 50}}));

    EXPECT_EQ(parse(protocol::getDialogsRequest()), json({{"type", "getDialogsRequest"}}));
    EXPECT_EQ(parse(protocol::ping()), json({{"type", "ping"}}));
}

TEST(ProtocolRequests, TokensAndOpenDialog) {
    EXPECT_EQ(parse(protocol::resumeConnectionRequest("abc")), json({{"type", "resumeConnectionRequest"}, {"token", "abc"}}));
    EXPECT_EQ(parse(protocol::resumeSessionRequest("xyz")), json({{"type", "resumeSessionRequest"}, {"token", "xyz"}}));
    EXPECT_EQ(parse(protocol::openDialogRequest(42)), json({{"type", "openDialogRequest"}, {"target_user_id", 42}}));
}

TEST(ProtocolRequests, EscapesSpecialCharacters) {
    const std::string tricky = "a\"b\\c\n{}";
    const auto j = parse(protocol::loginRequest(tricky, tricky));
    EXPECT_EQ(j["username"], tricky);
    EXPECT_EQ(j["password"], tricky);
}

TEST(ProtocolResponses, SendMessageWrapsMessageInData) {
    Message m;
    m.id = 1;
    m.dialog_id = 2;
    m.sender_id = 3;
    m.created_at_ms = 1760968800123;
    m.payload = TextContent{.text = "hi"};

    const auto j = parse(protocol::sendMessage(m));
    EXPECT_EQ(j["type"], "sendMessage");
    EXPECT_EQ(j["data"], json(m));
}

TEST(ProtocolResponses, LoginResponseCarriesTokensAndError) {
    const auto ok = parse(protocol::loginResponse(true, 7, "ivan", "conn", "sess"));
    EXPECT_EQ(ok, json({{"type", "loginResponse"}, {"success", true}, {"user_id", 7}, {"username", "ivan"},
                        {"connectToken", "conn"}, {"sessionToken", "sess"}, {"error", ""}}));

    const auto fail = parse(protocol::loginResponse(false, -1, "", "", "", "Invalid password"));
    EXPECT_EQ(fail["success"], false);
    EXPECT_EQ(fail["error"], "Invalid password");
}

TEST(ProtocolResponses, RegisterResponseHasSameShapeAsLogin) {
    const auto reg = parse(protocol::registerResponse(true, 7, "ivan", "conn", "sess"));
    auto login = parse(protocol::loginResponse(true, 7, "ivan", "conn", "sess"));
    login["type"] = "registerResponse";
    EXPECT_EQ(reg, login);
}

TEST(ProtocolResponses, SearchUserResponse) {
    const auto j = parse(protocol::searchUserResponse({{.username = "ivan", .user_id = 1}, {.username = "ivanova", .user_id = 2}}));
    EXPECT_EQ(j["type"], "searchUserResponse");
    EXPECT_EQ(j["result"], json::parse(R"([{"user_id":1,"username":"ivan"},{"user_id":2,"username":"ivanova"}])"));

    EXPECT_EQ(parse(protocol::searchUserResponse({}))["result"], json::array());
}

TEST(ProtocolResponses, ErrorMessage) {
    EXPECT_EQ(parse(protocol::errorMessage("Not authenticated")), json({{"type", "error"}, {"message", "Not authenticated"}}));
}

TEST(ProtocolResponses, HistoryResponse) {
    Message m;
    m.id = 10;
    m.payload = TextContent{.text = "x"};

    const auto j = parse(protocol::historyResponse(true, 99, {m}));
    EXPECT_EQ(j["type"], "historyResponse");
    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["dialog_id"], 99);
    ASSERT_EQ(j["messages"].size(), 1u);
    EXPECT_EQ(j["messages"][0]["id"], 10);
    EXPECT_EQ(j["error"], "");
}

TEST(ProtocolResponses, GetDialogsResponse) {
    MetaDialog_Client md;
    md.dialog_id = 5;
    md.peer_id = 3;
    md.username = "masha";

    const auto j = parse(protocol::getDialogsResponse(true, {md}));
    EXPECT_EQ(j["type"], "getDialogsResponse");
    ASSERT_EQ(j["dialogs"].size(), 1u);
    EXPECT_EQ(j["dialogs"][0]["username"], "masha");
}

TEST(ProtocolResponses, ResumeResponses) {
    EXPECT_EQ(parse(protocol::resumeConnectionResponse(true)), json({{"type", "resumeConnectionResponse"}, {"success", true}}));
    EXPECT_EQ(parse(protocol::resumeSessionResponse(true, "tok")), json({{"type", "resumeSessionResponse"}, {"success", true}, {"token", "tok"}}));
    EXPECT_EQ(parse(protocol::resumeSessionResponse(false))["token"], "");
}

TEST(ProtocolResponses, OpenDialogResponseSuccessHasDialogData) {
    const auto j = parse(protocol::openDialogResponse(true, 15, {.username = "masha", .user_id = 3}, {}));

    EXPECT_EQ(j["success"], true);
    EXPECT_EQ(j["dialog_id"], 15);
    EXPECT_EQ(j["peer"], json({{"user_id", 3}, {"username", "masha"}}));
    EXPECT_EQ(j["messages"], json::array());
    EXPECT_FALSE(j.contains("error"));
}

TEST(ProtocolResponses, OpenDialogResponseFailureHasOnlyError) {
    const auto j = parse(protocol::openDialogResponse(false, 0, User{}, {}, "User not found"));

    EXPECT_EQ(j["success"], false);
    EXPECT_EQ(j["error"], "User not found");
    EXPECT_FALSE(j.contains("dialog_id"));
    EXPECT_FALSE(j.contains("peer"));
}
