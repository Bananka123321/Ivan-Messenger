#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "HandlerTest.h"

using nlohmann::json;

namespace {

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

}

class HandlerMessagingTest : public HandlerTest {
protected:
    void SetUp() override {
        HandlerTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        ivanId_ = signUp(ivan_, "ivan");
        mashaId_ = signUp(masha_, "masha");
    }

    int64_t ivanOpensDialogWithMasha() { return openDialog(ivan_, mashaId_); }

    int messageCount() { return exec("SELECT COUNT(*) FROM messages")[0][0].as<int>(); }

    TestClient ivan_;
    TestClient masha_;
    int ivanId_ = 0;
    int mashaId_ = 0;
};

TEST_F(HandlerMessagingTest, SearchFindsUsersByPrefix) {
    send(ivan_, protocol::searchUserRequest("mas"));
    const auto response = ivan_.receiveOne();

    EXPECT_EQ(response["type"], "searchUserResponse");
    EXPECT_EQ(response["result"], json::parse(R"([{"user_id": )" + std::to_string(mashaId_) + R"(, "username": "masha"}])"));
}

TEST_F(HandlerMessagingTest, SearchWithEmptyQueryIsRejected) {
    send(ivan_, protocol::searchUserRequest(""));

    EXPECT_EQ(ivan_.receiveOne(), json({{"type", "error"}, {"message", "Cannot be empty\n"}}));
}

TEST_F(HandlerMessagingTest, SearchWithoutMatchesReturnsEmptyList) {
    send(ivan_, protocol::searchUserRequest("zzz"));

    EXPECT_EQ(ivan_.receiveOne()["result"], json::array());
}

TEST_F(HandlerMessagingTest, OpenDialogReturnsPeerAndEmptyHistory) {
    send(ivan_, protocol::openDialogRequest(mashaId_));
    const auto response = ivan_.receiveOne();

    EXPECT_EQ(response["type"], "openDialogResponse");
    EXPECT_EQ(response["success"], true);
    EXPECT_GT(response["dialog_id"].get<int64_t>(), 0);
    EXPECT_EQ(response["peer"], json({{"user_id", mashaId_}, {"username", "masha"}}));
    EXPECT_EQ(response["messages"], json::array());
}

TEST_F(HandlerMessagingTest, BothSidesOpenTheSameDialog) {
    EXPECT_EQ(openDialog(ivan_, mashaId_), openDialog(masha_, ivanId_));
}

TEST_F(HandlerMessagingTest, OpenDialogReturnsHistoryOldestFirst) {
    const auto dialogId = ivanOpensDialogWithMasha();
    send(ivan_, textMessage(dialogId, ivanId_, "первое"));
    send(ivan_, textMessage(dialogId, ivanId_, "второе"));
    masha_.receive();

    send(masha_, protocol::openDialogRequest(ivanId_));
    const auto messages = masha_.receiveOne()["messages"];

    ASSERT_EQ(messages.size(), 2u);
    EXPECT_EQ(messages[0]["payload"]["text"], "первое");
    EXPECT_EQ(messages[1]["payload"]["text"], "второе");
}

TEST_F(HandlerMessagingTest, OpenDialogErrors) {
    const std::vector<std::pair<std::string, std::string>> cases = {
        {protocol::openDialogRequest(ivanId_), "Cannot open dialog with yourself"},
        {protocol::openDialogRequest(99999), "User not found"},
        {R"({"type": "openDialogRequest", "target_user_id": "masha"})", "Invalid target_user_id"},
        {R"({"type": "openDialogRequest"})", "Invalid target_user_id"},
    };

    for (const auto& [request, error] : cases) {
        send(ivan_, request);
        const auto response = ivan_.receiveOne();
        EXPECT_EQ(response["success"], false) << request;
        EXPECT_EQ(response["error"], error) << request;
    }
}

TEST_F(HandlerMessagingTest, MessageIsStoredAndDeliveredToOnlinePeer) {
    const auto dialogId = ivanOpensDialogWithMasha();
    const int64_t before = nowMs();

    send(ivan_, textMessage(dialogId, ivanId_, "Привет, Маша!"));

    const auto delivered = masha_.receiveOne();
    EXPECT_EQ(delivered["type"], "sendMessage");
    const auto msg = delivered["data"].get<Message>();
    EXPECT_GT(msg.id, 0);
    EXPECT_EQ(msg.dialog_id, dialogId);
    EXPECT_EQ(msg.sender_id, ivanId_);
    EXPECT_EQ(std::get<TextContent>(msg.payload).text, "Привет, Маша!");
    EXPECT_GE(msg.created_at_ms, before);
    EXPECT_LE(msg.created_at_ms, nowMs());

    EXPECT_EQ(messageCount(), 1);
}

TEST_F(HandlerMessagingTest, SenderDoesNotReceiveOwnMessage) {
    const auto dialogId = ivanOpensDialogWithMasha();

    send(ivan_, textMessage(dialogId, ivanId_, "hi"));

    EXPECT_TRUE(ivan_.receive().empty());
}

TEST_F(HandlerMessagingTest, MessageToOfflinePeerIsKeptForHistory) {
    const auto dialogId = ivanOpensDialogWithMasha();
    sessions_.remove(masha_.session);

    send(ivan_, textMessage(dialogId, ivanId_, "ты где?"));

    EXPECT_TRUE(masha_.receive().empty());
    EXPECT_EQ(messageCount(), 1);
}

TEST_F(HandlerMessagingTest, MessageUpdatesDialogListOfBothUsers) {
    const auto dialogId = ivanOpensDialogWithMasha();
    send(ivan_, textMessage(dialogId, ivanId_, "последнее"));
    masha_.receive();

    for (auto* client : {&ivan_, &masha_}) {
        send(*client, protocol::getDialogsRequest());
        const auto response = client->receiveOne();
        ASSERT_EQ(response["dialogs"].size(), 1u);
        EXPECT_EQ(response["dialogs"][0]["dialog_id"], dialogId);
        EXPECT_EQ(response["dialogs"][0]["last_msg_preview"], "последнее");
    }
}

TEST_F(HandlerMessagingTest, ControlCharactersAreRemovedFromText) {
    const auto dialogId = ivanOpensDialogWithMasha();

    send(ivan_, textMessage(dialogId, ivanId_, std::string("при\x01вет\x7F", 13)));

    EXPECT_EQ(masha_.receiveOne()["data"]["payload"]["text"], "привет");
}

TEST_F(HandlerMessagingTest, TextOfOnlyForbiddenCharactersIsRejected) {
    const auto dialogId = ivanOpensDialogWithMasha();

    send(ivan_, textMessage(dialogId, ivanId_, "\x01\x02"));

    EXPECT_EQ(ivan_.receiveOne(), json({{"type", "error"}, {"message", "Message is empty"}}));
    EXPECT_EQ(messageCount(), 0);
}

TEST_F(HandlerMessagingTest, SpoofedSenderIsRejected) {
    const auto dialogId = ivanOpensDialogWithMasha();

    send(ivan_, textMessage(dialogId, mashaId_, "я Маша"));

    EXPECT_EQ(ivan_.receiveOne(), json({{"type", "error"}, {"message", "Forbidden: sender_id mismatch"}}));
    EXPECT_TRUE(masha_.receive().empty());
    EXPECT_EQ(messageCount(), 0);
}

TEST_F(HandlerMessagingTest, MessageToForeignDialogIsRejected) {
    const auto dialogId = ivanOpensDialogWithMasha();
    TestClient petr;
    const int petrId = signUp(petr, "petr");

    send(petr, textMessage(dialogId, petrId, "я тут"));

    EXPECT_EQ(petr.receiveOne(), json({{"type", "error"}, {"message", "You are not a participant of this dialog"}}));
    EXPECT_TRUE(ivan_.receive().empty());
    EXPECT_TRUE(masha_.receive().empty());
    EXPECT_EQ(messageCount(), 0);
}

TEST_F(HandlerMessagingTest, InvalidMessagesAreRejected) {
    const auto dialogId = ivanOpensDialogWithMasha();
    const std::vector<std::pair<std::string, std::string>> cases = {
        {R"({"type": "sendMessage"})", "Invalid message format"},
        {R"({"type": "sendMessage", "data": {"type": 0}})", "Invalid message format"},
        {textMessage(dialogId, ivanId_, ""), "Cannot be empty\n"},
        {textMessage(dialogId, ivanId_, std::string(4097, 'a')), "Too long(max 4096 characters)\n"},
    };

    for (const auto& [request, error] : cases) {
        send(ivan_, request);
        EXPECT_EQ(ivan_.receiveOne()["message"], error) << request.substr(0, 80);
    }
    EXPECT_EQ(messageCount(), 0);
}

TEST_F(HandlerMessagingTest, HistoryReturnsMessagesNewestFirst) {
    const auto dialogId = ivanOpensDialogWithMasha();
    for (const auto* text : {"1", "2", "3"})
        send(ivan_, textMessage(dialogId, ivanId_, text));

    send(masha_, protocol::historyRequest(dialogId, std::numeric_limits<int64_t>::max(), 2));
    const auto responses = masha_.receive();
    const auto& response = responses.back();

    EXPECT_EQ(response["type"], "historyResponse");
    EXPECT_EQ(response["success"], true);
    EXPECT_EQ(response["dialog_id"], dialogId);
    ASSERT_EQ(response["messages"].size(), 2u);
    EXPECT_EQ(response["messages"][0]["payload"]["text"], "3");
    EXPECT_EQ(response["messages"][1]["payload"]["text"], "2");
}

TEST_F(HandlerMessagingTest, HistoryOfForeignDialogIsForbidden) {
    const auto dialogId = ivanOpensDialogWithMasha();
    TestClient petr;
    signUp(petr, "petr");

    send(petr, protocol::historyRequest(dialogId, 1000, 10));

    EXPECT_EQ(petr.receiveOne()["message"], "You are not a participant of this dialog");
}

TEST_F(HandlerMessagingTest, HistoryValidatesParameters) {
    const auto dialogId = ivanOpensDialogWithMasha();
    const std::vector<std::pair<std::string, std::string>> cases = {
        {protocol::historyRequest(dialogId, 1000, 0), "Limit must be between 1 and 100"},
        {protocol::historyRequest(dialogId, 1000, 101), "Limit must be between 1 and 100"},
        {R"({"type": "historyRequest", "dialog_id": 1})", "Missing required fields"},
        {R"({"type": "historyRequest", "dialog_id": "1", "last_msg_id": 1, "limit": 10})", "Invalid parameter types"},
    };

    for (const auto& [request, error] : cases) {
        send(ivan_, request);
        EXPECT_EQ(ivan_.receiveOne()["message"], error) << request;
    }
}

TEST_F(HandlerMessagingTest, GetDialogsListsOpenedDialogs) {
    const auto dialogId = ivanOpensDialogWithMasha();

    send(masha_, protocol::getDialogsRequest());
    const auto response = masha_.receiveOne();

    EXPECT_EQ(response["type"], "getDialogsResponse");
    ASSERT_EQ(response["dialogs"].size(), 1u);
    EXPECT_EQ(response["dialogs"][0]["dialog_id"], dialogId);
    EXPECT_EQ(response["dialogs"][0]["peer_id"], ivanId_);
    EXPECT_EQ(response["dialogs"][0]["username"], "ivan");
}

TEST_F(HandlerMessagingTest, GetDialogsForNewUserIsEmpty) {
    send(ivan_, protocol::getDialogsRequest());

    EXPECT_EQ(ivan_.receiveOne()["dialogs"], json::array());
}
