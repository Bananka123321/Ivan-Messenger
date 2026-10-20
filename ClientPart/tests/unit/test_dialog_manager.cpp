#include <gtest/gtest.h>

#include <QSignalSpy>

#include "DialogManager.h"

namespace {

constexpr int64_t kTimestampMs = 1760968800123;

Message text(const int64_t id, const int64_t dialogId, const int senderId, const std::string& body, const int64_t ts = kTimestampMs) {
    Message m;
    m.id = id;
    m.dialog_id = static_cast<int>(dialogId);
    m.sender_id = senderId;
    m.created_at_ms = ts;
    m.payload = TextContent{.text = body};
    return m;
}

MetaDialog_Client meta(const int64_t dialogId, const int peerId, const std::string& name, const int64_t ts) {
    MetaDialog_Client md;
    md.dialog_id = dialogId;
    md.peer_id = peerId;
    md.username = name;
    md.last_msg_preview = "preview " + name;
    md.last_msg_timestamp = ts;
    return md;
}

std::vector<int64_t> ids(const std::vector<Message>* messages) {
    std::vector<int64_t> result;
    if (messages)
        for (const auto& m : *messages)
            result.push_back(m.id);
    return result;
}

}

class DialogManagerTest : public testing::Test {
protected:
    void SetUp() override {
        QSettings settings("IvanMessenger", "ClientConfig");
        settings.clear();
        state_.saveSession("token", 7, "ivan");
    }

    AppState state_;
    Handler handler_;
    DialogManager dm_{&handler_, &state_};
};

TEST_F(DialogManagerTest, MessageInUnknownDialogCreatesItAndAsksForDetails) {
    QSignalSpy unknown(&dm_, &DialogManager::unknownDialog);
    QSignalSpy messages(&dm_, &DialogManager::messagesUpdated);
    QSignalSpy dialogs(&dm_, &DialogManager::dialogsUpdated);

    dm_.addMessage(15, text(1, 15, 3, "Привет"));

    ASSERT_EQ(unknown.count(), 1);
    EXPECT_EQ(unknown.at(0).at(0).value<int64_t>(), 15);
    EXPECT_EQ(messages.count(), 1);
    EXPECT_EQ(dialogs.count(), 1);
    EXPECT_EQ(ids(dm_.getMessages(15)), std::vector<int64_t>{1});
}

TEST_F(DialogManagerTest, MessageInKnownDialogIsAppended) {
    dm_.addMessage(15, text(1, 15, 3, "a"));
    QSignalSpy unknown(&dm_, &DialogManager::unknownDialog);

    dm_.addMessage(15, text(2, 15, 7, "b"));

    EXPECT_EQ(unknown.count(), 0);
    EXPECT_EQ(ids(dm_.getMessages(15)), (std::vector<int64_t>{1, 2}));
}

TEST_F(DialogManagerTest, DuplicateMessageIsIgnored) {
    dm_.addMessage(15, text(1, 15, 3, "a"));
    QSignalSpy messages(&dm_, &DialogManager::messagesUpdated);

    dm_.addMessage(15, text(1, 15, 3, "a"));

    EXPECT_EQ(messages.count(), 0);
    EXPECT_EQ(dm_.getMessages(15)->size(), 1u);
}

TEST_F(DialogManagerTest, LocalMessagesWithoutIdAreNotDeduplicated) {
    dm_.addMessage(15, text(0, 15, 7, "раз"));
    dm_.addMessage(15, text(0, 15, 7, "раз"));

    EXPECT_EQ(dm_.getMessages(15)->size(), 2u);
}

TEST_F(DialogManagerTest, MessageUpdatesDialogPreview) {
    dm_.addMessage(15, text(1, 15, 3, "последнее", kTimestampMs + 10));

    const auto dialogs = dm_.getDialogs();
    ASSERT_EQ(dialogs.size(), 1u);
    EXPECT_EQ(dialogs[0].dialog_id, 15);
    EXPECT_EQ(dialogs[0].last_msg_preview, "последнее");
    EXPECT_EQ(dialogs[0].last_msg_timestamp, kTimestampMs + 10);
}

TEST_F(DialogManagerTest, UnknownDialogHasNoMessages) {
    EXPECT_EQ(dm_.getMessages(404), nullptr);
}

TEST_F(DialogManagerTest, PrependHistoryRestoresChronologicalOrder) {
    dm_.addMessage(15, text(4, 15, 3, "новое"));

    dm_.prependHistory(15, {text(3, 15, 3, "3"), text(2, 15, 3, "2"), text(1, 15, 3, "1")});

    EXPECT_EQ(ids(dm_.getMessages(15)), (std::vector<int64_t>{1, 2, 3, 4}));
}

TEST_F(DialogManagerTest, PrependHistorySkipsKnownMessages) {
    dm_.addMessage(15, text(2, 15, 3, "2"));

    dm_.prependHistory(15, {text(2, 15, 3, "2"), text(1, 15, 3, "1")});

    EXPECT_EQ(ids(dm_.getMessages(15)), (std::vector<int64_t>{1, 2}));
}

TEST_F(DialogManagerTest, SetHistoryReplacesMessages) {
    dm_.addMessage(15, text(9, 15, 3, "старое"));
    QSignalSpy dialogs(&dm_, &DialogManager::dialogsUpdated);

    dm_.setHistory(15, {text(1, 15, 3, "a"), text(2, 15, 3, "b")});

    EXPECT_EQ(ids(dm_.getMessages(15)), (std::vector<int64_t>{1, 2}));
    EXPECT_EQ(dialogs.count(), 1);
    EXPECT_EQ(dm_.getDialogs()[0].last_msg_preview, "b");
}

TEST_F(DialogManagerTest, SetHistoryWithEmptyListKeepsMessages) {
    dm_.addMessage(15, text(1, 15, 3, "a"));

    dm_.setHistory(15, {});

    EXPECT_EQ(dm_.getMessages(15)->size(), 1u);
}

TEST_F(DialogManagerTest, DialogsLoadedFillCacheAndUserDirectory) {
    QSignalSpy dialogs(&dm_, &DialogManager::dialogsUpdated);

    dm_.dialogsLoaded({meta(15, 3, "masha", 100), meta(16, 4, "petr", 200)});

    EXPECT_EQ(dialogs.count(), 1);
    EXPECT_EQ(dm_.getDialogs().size(), 2u);
    EXPECT_EQ(state_.getUsernameByUserId(3), "masha");
    EXPECT_EQ(state_.getUsernameByUserId(4), "petr");
}

TEST_F(DialogManagerTest, DialogsAreSortedByLastMessageTime) {
    dm_.dialogsLoaded({meta(15, 3, "old", 100), meta(16, 4, "new", 300), meta(17, 5, "mid", 200)});

    const auto dialogs = dm_.getDialogs();

    ASSERT_EQ(dialogs.size(), 3u);
    EXPECT_EQ(dialogs[0].username, "new");
    EXPECT_EQ(dialogs[1].username, "mid");
    EXPECT_EQ(dialogs[2].username, "old");
}

TEST_F(DialogManagerTest, SetCurrentDialogWithHistory) {
    QSignalSpy current(&dm_, &DialogManager::currentDialogChanged);

    dm_.setCurrentDialog(15, {.username = "masha", .user_id = 3}, {text(1, 15, 3, "a"), text(2, 15, 7, "b", kTimestampMs + 5)});

    ASSERT_EQ(current.count(), 1);
    EXPECT_EQ(current.at(0).at(0).value<int64_t>(), 15);
    const auto dialogs = dm_.getDialogs();
    ASSERT_EQ(dialogs.size(), 1u);
    EXPECT_EQ(dialogs[0].peer_id, 3);
    EXPECT_EQ(dialogs[0].username, "masha");
    EXPECT_EQ(dialogs[0].last_msg_preview, "b");
    EXPECT_EQ(dialogs[0].last_msg_timestamp, kTimestampMs + 5);
    EXPECT_EQ(ids(dm_.getMessages(15)), (std::vector<int64_t>{1, 2}));
}

TEST_F(DialogManagerTest, SetCurrentDialogWithoutHistoryShowsInvitation) {
    dm_.setCurrentDialog(15, {.username = "masha", .user_id = 3}, {});

    EXPECT_EQ(dm_.getDialogs()[0].last_msg_preview, "Начните общение...");
    EXPECT_TRUE(dm_.getMessages(15)->empty());
}

TEST_F(DialogManagerTest, MessagesForQmlAreNewestFirstAndMarkMine) {
    dm_.addMessage(15, text(1, 15, 3, "от Маши", 100));
    dm_.addMessage(15, text(2, 15, 7, "моё", 200));

    const auto list = dm_.messagesForQml(15);

    ASSERT_EQ(list.size(), 2);
    const auto newest = list[0].toMap();
    EXPECT_EQ(newest["msgId"].toLongLong(), 2);
    EXPECT_EQ(newest["messageText"].toString(), "моё");
    EXPECT_TRUE(newest["isMine"].toBool());
    EXPECT_EQ(newest["timestamp"].toLongLong(), 200);
    EXPECT_FALSE(list[1].toMap()["isMine"].toBool());
}

TEST_F(DialogManagerTest, MessagesForUnknownDialogAreEmpty) {
    EXPECT_TRUE(dm_.messagesForQml(404).isEmpty());
}

TEST_F(DialogManagerTest, DialogsForQml) {
    dm_.dialogsLoaded({meta(15, 3, "masha", 0)});

    const auto list = dm_.dialogsForQml();

    ASSERT_EQ(list.size(), 1);
    const auto d = list[0].toMap();
    EXPECT_EQ(d["chatId"].toLongLong(), 15);
    EXPECT_EQ(d["peerId"].toInt(), 3);
    EXPECT_EQ(d["name"].toString(), "masha");
    EXPECT_EQ(d["lastMsg"].toString(), "preview masha");
    EXPECT_EQ(d["time"].toString(), "") << "без сообщений время не показывается";
}

TEST_F(DialogManagerTest, StartConnectsToHandlerSignals) {
    dm_.start();
    QSignalSpy history(&dm_, &DialogManager::historyLoaded);
    QSignalSpy found(&dm_, &DialogManager::findUsers);

    handler_.handleMessage(protocol::sendMessage(text(5, 15, 3, "live")));
    handler_.handleMessage(protocol::historyResponse(true, 15, {text(4, 15, 3, "4"), text(3, 15, 3, "3")}));
    handler_.handleMessage(protocol::searchUserResponse({{.username = "masha", .user_id = 3}}));
    handler_.handleMessage(protocol::getDialogsResponse(true, {meta(16, 4, "petr", 1)}));

    EXPECT_EQ(ids(dm_.getMessages(15)), (std::vector<int64_t>{3, 4, 5}));
    EXPECT_EQ(history.count(), 1);
    EXPECT_EQ(found.count(), 1);
    EXPECT_EQ(dm_.getDialogs().size(), 2u);
}
