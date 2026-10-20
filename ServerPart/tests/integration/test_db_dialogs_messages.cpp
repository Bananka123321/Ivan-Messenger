#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <limits>
#include <thread>

#include "DB_DialogManager.h"
#include "DB_MessageManager.h"
#include "TestDatabase.h"

using testing::ElementsAre;
using testing::UnorderedElementsAre;

namespace {

constexpr int64_t kNoUpperBound = std::numeric_limits<int64_t>::max();
constexpr int64_t kTimestampMs = 1760968800123;

Message text(const int64_t dialogId, const int senderId, const std::string& body, const int64_t ts = kTimestampMs) {
    Message m;
    m.dialog_id = static_cast<int>(dialogId);
    m.sender_id = senderId;
    m.type = MessageType::Text;
    m.created_at_ms = ts;
    m.payload = TextContent{.text = body};
    return m;
}

}

class DBDialogsMessagesTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        dialogs_ = std::make_unique<DB_DialogManager>(TestDatabase::connectionString());
        messages_ = std::make_unique<DB_MessageManager>(TestDatabase::connectionString());
        ivan_ = insertUser("ivan");
        masha_ = insertUser("masha");
    }

    DB_DialogManager& dialogs() { return *dialogs_; }
    DB_MessageManager& messages() { return *messages_; }

    int64_t openDialog() {
        const auto id = dialogs().findOrCreatePrivateDialog(ivan_, masha_);
        EXPECT_TRUE(id.has_value());
        return id.value_or(0);
    }

    Message send(const int64_t dialogId, const int senderId, const std::string& body, const int64_t ts = kTimestampMs) {
        Message m = text(dialogId, senderId, body, ts);
        const auto id = messages().saveMessage(m);
        EXPECT_TRUE(id.has_value());
        m.id = id.value_or(0);
        dialogs().updateLastMessage(m);
        return m;
    }

    int ivan_ = 0;
    int masha_ = 0;

private:
    std::unique_ptr<DB_DialogManager> dialogs_;
    std::unique_ptr<DB_MessageManager> messages_;
};

TEST_F(DBDialogsMessagesTest, CreatesDialogWithBothParticipants) {
    const auto id = openDialog();

    EXPECT_GT(id, 0);
    EXPECT_THAT(dialogs().getDialogParticipants(id), UnorderedElementsAre(ivan_, masha_));
}

TEST_F(DBDialogsMessagesTest, ReturnsExistingDialogInsteadOfCreatingNew) {
    const auto first = openDialog();

    EXPECT_EQ(dialogs().findOrCreatePrivateDialog(ivan_, masha_), first);
    EXPECT_EQ(dialogs().findOrCreatePrivateDialog(masha_, ivan_), first);
    EXPECT_EQ(exec("SELECT COUNT(*) FROM dialogs")[0][0].as<int>(), 1);
}

TEST_F(DBDialogsMessagesTest, DifferentPairsGetDifferentDialogs) {
    const int petr = insertUser("petr");

    const auto withMasha = dialogs().findOrCreatePrivateDialog(ivan_, masha_);
    const auto withPetr = dialogs().findOrCreatePrivateDialog(ivan_, petr);

    ASSERT_TRUE(withMasha && withPetr);
    EXPECT_NE(*withMasha, *withPetr);
}

TEST_F(DBDialogsMessagesTest, ConcurrentOpeningCreatesSingleDialog) {
    constexpr int kThreads = 8;
    std::vector<std::optional<int64_t>> results(kThreads);
    std::vector<std::thread> threads;

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            DB_DialogManager manager(TestDatabase::connectionString());
            results[i] = i % 2 ? manager.findOrCreatePrivateDialog(ivan_, masha_)
                               : manager.findOrCreatePrivateDialog(masha_, ivan_);
        });
    }
    for (auto& t : threads)
        t.join();

    for (const auto& r : results) {
        ASSERT_TRUE(r.has_value());
        EXPECT_EQ(*r, *results[0]);
    }
    EXPECT_EQ(exec("SELECT COUNT(*) FROM dialogs")[0][0].as<int>(), 1);
}

TEST_F(DBDialogsMessagesTest, UnknownUserCannotGetDialog) {
    EXPECT_EQ(dialogs().findOrCreatePrivateDialog(ivan_, 99999), std::nullopt);
}

TEST_F(DBDialogsMessagesTest, ParticipantsOfUnknownDialogAreEmpty) {
    EXPECT_TRUE(dialogs().getDialogParticipants(424242).empty());
}

TEST_F(DBDialogsMessagesTest, UserDialogsShowPeerAndLastMessage) {
    const auto id = openDialog();
    send(id, masha_, "Привет!", kTimestampMs);

    const auto list = dialogs().getUserDialogs(ivan_);

    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].dialog_id, id);
    EXPECT_EQ(list[0].peer_id, masha_);
    EXPECT_EQ(list[0].username, "masha");
    EXPECT_EQ(list[0].last_msg_preview, "Привет!");
    EXPECT_EQ(list[0].last_msg_timestamp, kTimestampMs);
    EXPECT_EQ(list[0].display_name, "masha");
}

TEST_F(DBDialogsMessagesTest, DisplayNameIsYouWhenLastMessageIsMine) {
    const auto id = openDialog();
    send(id, ivan_, "Как дела?");

    EXPECT_EQ(dialogs().getUserDialogs(ivan_)[0].display_name, "Вы");
    EXPECT_EQ(dialogs().getUserDialogs(masha_)[0].display_name, "ivan");
}

TEST_F(DBDialogsMessagesTest, LastMessageIsReplacedByNewerOne) {
    const auto id = openDialog();
    send(id, ivan_, "первое", kTimestampMs);
    send(id, masha_, "второе", kTimestampMs + 1000);

    const auto list = dialogs().getUserDialogs(ivan_);
    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].last_msg_preview, "второе");
    EXPECT_EQ(list[0].last_msg_timestamp, kTimestampMs + 1000);
}

TEST_F(DBDialogsMessagesTest, NewDialogWithoutMessagesHasEmptyPreview) {
    openDialog();

    const auto list = dialogs().getUserDialogs(masha_);

    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].peer_id, ivan_);
    EXPECT_EQ(list[0].last_msg_preview, "");
    EXPECT_EQ(list[0].last_msg_timestamp, 0);
}

TEST_F(DBDialogsMessagesTest, UserSeesOnlyOwnDialogs) {
    const int petr = insertUser("petr");
    openDialog();
    dialogs().findOrCreatePrivateDialog(masha_, petr);

    const auto list = dialogs().getUserDialogs(ivan_);

    ASSERT_EQ(list.size(), 1u);
    EXPECT_EQ(list[0].peer_id, masha_);
    EXPECT_EQ(dialogs().getUserDialogs(masha_).size(), 2u);
}

TEST_F(DBDialogsMessagesTest, SavedMessagesGetIncreasingIds) {
    const auto id = openDialog();

    const auto a = send(id, ivan_, "a");
    const auto b = send(id, ivan_, "b");

    EXPECT_GT(a.id, 0);
    EXPECT_GT(b.id, a.id);
}

TEST_F(DBDialogsMessagesTest, HistoryReturnsNewestFirstWithAllFields) {
    const auto id = openDialog();
    const auto first = send(id, ivan_, "первое", kTimestampMs);
    const auto second = send(id, masha_, "второе", kTimestampMs + 5);

    const auto history = messages().getHistory(id, kNoUpperBound, 10);

    ASSERT_EQ(history.size(), 2u);
    EXPECT_EQ(history[0].id, second.id);
    EXPECT_EQ(history[1].id, first.id);

    EXPECT_EQ(history[0].dialog_id, id);
    EXPECT_EQ(history[0].sender_id, masha_);
    EXPECT_EQ(history[0].type, MessageType::Text);
    EXPECT_EQ(history[0].created_at_ms, kTimestampMs + 5);
    EXPECT_EQ(std::get<TextContent>(history[0].payload).text, "второе");
}

TEST_F(DBDialogsMessagesTest, HistoryPaginatesWithBeforeIdAndLimit) {
    const auto id = openDialog();
    std::vector<int64_t> ids;
    for (int i = 0; i < 5; ++i)
        ids.push_back(send(id, ivan_, std::to_string(i)).id);

    const auto page1 = messages().getHistory(id, kNoUpperBound, 2);
    const auto page2 = messages().getHistory(id, page1.back().id, 2);
    const auto page3 = messages().getHistory(id, page2.back().id, 2);

    auto idsOf = [](const std::vector<Message>& v) {
        std::vector<int64_t> r;
        for (const auto& m : v)
            r.push_back(m.id);
        return r;
    };
    EXPECT_THAT(idsOf(page1), ElementsAre(ids[4], ids[3]));
    EXPECT_THAT(idsOf(page2), ElementsAre(ids[2], ids[1]));
    EXPECT_THAT(idsOf(page3), ElementsAre(ids[0]));
}

TEST_F(DBDialogsMessagesTest, HistoryContainsOnlyRequestedDialog) {
    const int petr = insertUser("petr");
    const auto withMasha = openDialog();
    const auto withPetr = dialogs().findOrCreatePrivateDialog(ivan_, petr).value();
    send(withMasha, ivan_, "Маше");
    send(withPetr, ivan_, "Пете");

    const auto history = messages().getHistory(withPetr, kNoUpperBound, 10);

    ASSERT_EQ(history.size(), 1u);
    EXPECT_EQ(std::get<TextContent>(history[0].payload).text, "Пете");
}

TEST_F(DBDialogsMessagesTest, MediaAndVoicePayloadsSurviveStorage) {
    const auto id = openDialog();

    Message photo = text(id, ivan_, "");
    photo.type = MessageType::Image;
    photo.payload = MediaContent{.url = "files/1", .caption = "закат", .filename = "sunset.jpg", .size_bytes = 123456};

    Message voice = text(id, ivan_, "");
    voice.type = MessageType::Voice;
    voice.payload = VoiceContent{.url = "voice/1", .duration_sec = 12};

    ASSERT_TRUE(messages().saveMessage(photo));
    ASSERT_TRUE(messages().saveMessage(voice));

    const auto history = messages().getHistory(id, kNoUpperBound, 10);
    ASSERT_EQ(history.size(), 2u);

    const auto& v = std::get<VoiceContent>(history[0].payload);
    EXPECT_EQ(v.url, "voice/1");
    EXPECT_EQ(v.duration_sec, 12u);

    EXPECT_EQ(history[1].type, MessageType::Image);
    const auto& m = std::get<MediaContent>(history[1].payload);
    EXPECT_EQ(m.url, "files/1");
    EXPECT_EQ(m.caption, "закат");
    EXPECT_EQ(m.filename, "sunset.jpg");
    EXPECT_EQ(m.size_bytes, 123456u);
}

TEST_F(DBDialogsMessagesTest, TextWithSpecialCharactersIsStoredAsIs) {
    const auto id = openDialog();
    const std::string body = "кавычки \" ' \\ и эмодзи 😀\nновая строка";
    send(id, ivan_, body);

    const auto history = messages().getHistory(id, kNoUpperBound, 1);

    EXPECT_EQ(std::get<TextContent>(history[0].payload).text, body);
}

TEST_F(DBDialogsMessagesTest, SavingToUnknownDialogFails) {
    EXPECT_EQ(messages().saveMessage(text(424242, ivan_, "x")), std::nullopt);
}

TEST_F(DBDialogsMessagesTest, MessagesAreDeletedWithDialog) {
    const auto id = openDialog();
    send(id, ivan_, "x");

    exec("DELETE FROM dialogs");

    EXPECT_EQ(exec("SELECT COUNT(*) FROM messages")[0][0].as<int>(), 0);
    EXPECT_EQ(exec("SELECT COUNT(*) FROM dialog_participants")[0][0].as<int>(), 0);
}
