#include <gtest/gtest.h>

#include <QSignalSpy>
#include <fstream>

#include "Handler.h"

using nlohmann::json;

namespace {

json fixture(const std::string& name) {
    std::ifstream file(std::string(CONTRACT_DIR) + "/" + name + ".json");
    if (!file)
        throw std::runtime_error("Нет эталонного пакета: " + name);
    return json::parse(file);
}

template <typename T>
T arg(const QSignalSpy& spy, const int index) {
    return spy.at(0).at(index).value<T>();
}

void expectSameMessage(const Message& actual, const json& expected) {
    EXPECT_EQ(actual.id, expected["id"].get<int64_t>());
    EXPECT_EQ(actual.dialog_id, expected["dialog_id"].get<int>());
    EXPECT_EQ(actual.sender_id, expected["sender_id"].get<int>());
    EXPECT_EQ(static_cast<int>(actual.type), expected["type"].get<int>());
    EXPECT_EQ(actual.created_at_ms, expected["timestamp"].get<int64_t>());
    EXPECT_EQ(std::get<TextContent>(actual.payload).text, expected["payload"]["text"].get<std::string>());
}

}

TEST(ClientContract, BuildsRequestsLikeFixtures) {
    const auto login = fixture("client_to_server/login_request");
    EXPECT_EQ(json::parse(protocol::loginRequest(login["username"], login["password"])), login);

    const auto reg = fixture("client_to_server/register_request");
    EXPECT_EQ(json::parse(protocol::registerRequest(reg["username"], reg["password"])), reg);

    const auto search = fixture("client_to_server/search_user_request");
    EXPECT_EQ(json::parse(protocol::searchUserRequest(search["username"])), search);

    const auto dialogs = fixture("client_to_server/get_dialogs_request");
    EXPECT_EQ(json::parse(protocol::getDialogsRequest()), dialogs);

    EXPECT_EQ(json::parse(protocol::ping()), fixture("client_to_server/ping"));

    const auto openDialog = fixture("client_to_server/open_dialog_request");
    EXPECT_EQ(json::parse(protocol::openDialogRequest(openDialog["target_user_id"])), openDialog);
}

TEST(ClientContract, BuildsHistoryRequestWithClientLimit) {
    const auto f = fixture("client_to_server/history_request");

    EXPECT_EQ(f["limit"], protocol::HISTORY_MAX_LIMIT);
    EXPECT_EQ(json::parse(protocol::historyRequest(f["dialog_id"], f["last_msg_id"], protocol::HISTORY_MAX_LIMIT)), f);
}

TEST(ClientContract, BuildsResumeRequests) {
    const auto connection = fixture("client_to_server/resume_connection_request");
    EXPECT_EQ(json::parse(protocol::resumeConnectionRequest(connection["token"])), connection);

    const auto session = fixture("client_to_server/resume_session_request");
    EXPECT_EQ(json::parse(protocol::resumeSessionRequest(session["token"])), session);
}

TEST(ClientContract, BuildsOutgoingMessage) {
    const auto f = fixture("client_to_server/send_message");

    Message msg;
    msg.dialog_id = f["data"]["dialog_id"];
    msg.sender_id = f["data"]["sender_id"];
    msg.type = MessageType::Text;
    msg.created_at_ms = f["data"]["timestamp"];
    msg.payload = TextContent{.text = f["data"]["payload"]["text"]};

    EXPECT_EQ(json::parse(protocol::sendMessage(msg)), f);
}

class ClientContractResponses : public testing::Test {
protected:
    void receive(const json& packet) { handler_.handleMessage(packet.dump()); }

    Handler handler_;
};

TEST_F(ClientContractResponses, LoginSuccess) {
    const auto f = fixture("server_to_client/login_response_success");
    QSignalSpy spy(&handler_, &Handler::S_loginSuccess);

    receive(f);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(arg<std::string>(spy, 0), f["username"].get<std::string>());
    EXPECT_EQ(arg<int>(spy, 1), f["user_id"]);
    EXPECT_EQ(arg<std::string>(spy, 2), f["connectToken"].get<std::string>());
    EXPECT_EQ(arg<std::string>(spy, 3), f["sessionToken"].get<std::string>());
}

TEST_F(ClientContractResponses, LoginAndRegisterFailuresShowServerReason) {
    QSignalSpy login(&handler_, &Handler::S_loginFailed);
    QSignalSpy reg(&handler_, &Handler::S_registerFailed);

    receive(fixture("server_to_client/login_response_failure"));
    receive(fixture("server_to_client/register_response_failure"));

    ASSERT_EQ(login.count(), 1);
    EXPECT_EQ(arg<std::string>(login, 0), "Invalid password");
    ASSERT_EQ(reg.count(), 1);
    EXPECT_EQ(arg<std::string>(reg, 0), "Username is already taken");
}

TEST_F(ClientContractResponses, RegisterSuccess) {
    const auto f = fixture("server_to_client/register_response_success");
    QSignalSpy spy(&handler_, &Handler::S_registerSuccess);

    receive(f);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(arg<int>(spy, 1), f["user_id"]);
    EXPECT_EQ(arg<std::string>(spy, 3), f["sessionToken"].get<std::string>());
}

TEST_F(ClientContractResponses, SearchResults) {
    const auto f = fixture("server_to_client/search_user_response");
    QSignalSpy spy(&handler_, &Handler::S_UserSearch);

    receive(f);

    ASSERT_EQ(spy.count(), 1);
    const auto users = arg<std::vector<User>>(spy, 0);
    ASSERT_EQ(users.size(), f["result"].size());
    for (size_t i = 0; i < users.size(); ++i) {
        EXPECT_EQ(users[i].user_id, f["result"][i]["user_id"]);
        EXPECT_EQ(users[i].username, f["result"][i]["username"].get<std::string>());
    }
}

TEST_F(ClientContractResponses, History) {
    const auto f = fixture("server_to_client/history_response");
    QSignalSpy spy(&handler_, &Handler::S_HistoryLoaded);

    receive(f);

    ASSERT_EQ(spy.count(), 1);
    EXPECT_EQ(arg<int64_t>(spy, 0), f["dialog_id"]);
    const auto messages = arg<std::vector<Message>>(spy, 1);
    ASSERT_EQ(messages.size(), f["messages"].size());
    for (size_t i = 0; i < messages.size(); ++i)
        expectSameMessage(messages[i], f["messages"][i]);
}

TEST_F(ClientContractResponses, Dialogs) {
    const auto f = fixture("server_to_client/get_dialogs_response");
    QSignalSpy spy(&handler_, &Handler::S_DialogsLoaded);

    receive(f);

    ASSERT_EQ(spy.count(), 1);
    const auto dialogs = arg<std::vector<MetaDialog_Client>>(spy, 0);
    ASSERT_EQ(dialogs.size(), 1u);
    const auto& expected = f["dialogs"][0];
    EXPECT_EQ(dialogs[0].dialog_id, expected["dialog_id"]);
    EXPECT_EQ(dialogs[0].peer_id, expected["peer_id"]);
    EXPECT_EQ(dialogs[0].username, expected["username"].get<std::string>());
    EXPECT_EQ(dialogs[0].last_msg_preview, expected["last_msg_preview"].get<std::string>());
    EXPECT_EQ(dialogs[0].last_msg_timestamp, expected["last_msg_timestamp"]);
}

TEST_F(ClientContractResponses, ResumeResponses) {
    QSignalSpy connection(&handler_, &Handler::S_ResumeConnectionSuccess);
    QSignalSpy session(&handler_, &Handler::S_ResumeSessionResponse);
    const auto sessionFixture = fixture("server_to_client/resume_session_response");

    receive(fixture("server_to_client/resume_connection_response"));
    receive(sessionFixture);

    EXPECT_EQ(connection.count(), 1);
    ASSERT_EQ(session.count(), 1);
    EXPECT_TRUE(arg<bool>(session, 0));
    EXPECT_EQ(arg<std::string>(session, 1), sessionFixture["token"].get<std::string>());
}

TEST_F(ClientContractResponses, OpenDialog) {
    const auto ok = fixture("server_to_client/open_dialog_response_success");
    QSignalSpy spy(&handler_, &Handler::S_DialogOpened);

    receive(ok);
    receive(fixture("server_to_client/open_dialog_response_failure"));

    ASSERT_EQ(spy.count(), 2);
    EXPECT_TRUE(spy.at(0).at(0).toBool());
    EXPECT_EQ(spy.at(0).at(1).value<int64_t>(), ok["dialog_id"]);
    EXPECT_EQ(spy.at(0).at(2).value<User>().username, ok["peer"]["username"].get<std::string>());
    const auto messages = spy.at(0).at(3).value<std::vector<Message>>();
    ASSERT_EQ(messages.size(), 1u);
    expectSameMessage(messages[0], ok["messages"][0]);

    EXPECT_FALSE(spy.at(1).at(0).toBool());
    EXPECT_EQ(spy.at(1).at(4).value<std::string>(), "User not found");
}

TEST_F(ClientContractResponses, IncomingMessage) {
    const auto f = fixture("server_to_client/send_message");
    QSignalSpy spy(&handler_, &Handler::S_Message);

    receive(f);

    ASSERT_EQ(spy.count(), 1);
    expectSameMessage(arg<Message>(spy, 0), f["data"]);
}

TEST_F(ClientContractResponses, ErrorIsUnderstood) {
    QSignalSpy message(&handler_, &Handler::S_Message);

    receive(fixture("server_to_client/error"));

    EXPECT_EQ(message.count(), 0);
}
