#include <gtest/gtest.h>

#include "protocol.h"

namespace {

constexpr int64_t kTimestampMs = 1760968800123;
constexpr int64_t kBigId = 5'000'000'000;

Message textMessage(const std::string& text) {
    Message m;
    m.id = kBigId;
    m.dialog_id = 12;
    m.sender_id = 7;
    m.type = MessageType::Text;
    m.created_at_ms = kTimestampMs;
    m.payload = TextContent{.text = text};
    return m;
}

Message roundTrip(const Message& m) {
    const nlohmann::json j = m;
    return nlohmann::json::parse(j.dump()).get<Message>();
}

}

TEST(MessagePreview, ReturnsTextForTextMessage) {
    EXPECT_EQ(textMessage("Привет").getPreview(), "Привет");
}

TEST(MessagePreview, ReturnsPlaceholderForMedia) {
    Message m;
    m.payload = MediaContent{.url = "u", .filename = "photo.jpg"};
    EXPECT_EQ(m.getPreview(), "Файл...");
}

TEST(MessagePreview, ReturnsDurationForVoice) {
    Message m;
    m.payload = VoiceContent{.url = "u", .duration_sec = 42};
    EXPECT_EQ(m.getPreview(), "42 сек");
}

TEST(MessageType, NumericValuesAreStable) {
    EXPECT_EQ(static_cast<int>(MessageType::Text), 0);
    EXPECT_EQ(static_cast<int>(MessageType::Image), 1);
    EXPECT_EQ(static_cast<int>(MessageType::Voice), 2);
    EXPECT_EQ(static_cast<int>(MessageType::File), 3);
    EXPECT_EQ(static_cast<int>(MessageType::Sticker), 4);
}

TEST(MessageJson, SerializesAllFields) {
    const nlohmann::json j = textMessage("hi");

    EXPECT_EQ(j["id"], kBigId);
    EXPECT_EQ(j["dialog_id"], 12);
    EXPECT_EQ(j["sender_id"], 7);
    EXPECT_EQ(j["type"], 0);
    EXPECT_EQ(j["timestamp"], kTimestampMs);
    EXPECT_EQ(j["payload"], nlohmann::json({{"text", "hi"}}));
}

TEST(MessageJson, TextRoundTripKeeps64BitIdAndTimestamp) {
    const Message original = textMessage("Привет 👋");
    const Message restored = roundTrip(original);

    EXPECT_EQ(restored.id, kBigId);
    EXPECT_EQ(restored.dialog_id, 12);
    EXPECT_EQ(restored.sender_id, 7);
    EXPECT_EQ(restored.type, MessageType::Text);
    EXPECT_EQ(restored.created_at_ms, kTimestampMs);
    ASSERT_TRUE(std::holds_alternative<TextContent>(restored.payload));
    EXPECT_EQ(std::get<TextContent>(restored.payload).text, "Привет 👋");
}

TEST(MessageJson, MediaRoundTrip) {
    for (const auto type : {MessageType::Image, MessageType::File}) {
        Message m;
        m.type = type;
        m.created_at_ms = kTimestampMs;
        m.payload = MediaContent{.url = "files/abc", .caption = "смотри", .filename = "photo.jpg", .size_bytes = 6'000'000'000ULL};

        const Message restored = roundTrip(m);

        EXPECT_EQ(restored.type, type);
        ASSERT_TRUE(std::holds_alternative<MediaContent>(restored.payload));
        const auto& media = std::get<MediaContent>(restored.payload);
        EXPECT_EQ(media.url, "files/abc");
        EXPECT_EQ(media.caption, "смотри");
        EXPECT_EQ(media.filename, "photo.jpg");
        EXPECT_EQ(media.size_bytes, 6'000'000'000ULL);
    }
}

TEST(MessageJson, VoiceRoundTrip) {
    Message m;
    m.type = MessageType::Voice;
    m.payload = VoiceContent{.url = "voice/1", .duration_sec = 17};

    const Message restored = roundTrip(m);

    ASSERT_TRUE(std::holds_alternative<VoiceContent>(restored.payload));
    EXPECT_EQ(std::get<VoiceContent>(restored.payload).url, "voice/1");
    EXPECT_EQ(std::get<VoiceContent>(restored.payload).duration_sec, 17u);
}

TEST(MessageJson, MissingOptionalFieldsGetDefaults) {
    const auto j = nlohmann::json::parse(R"({"type": 0, "payload": {}})");
    const auto m = j.get<Message>();

    EXPECT_EQ(m.id, 0);
    EXPECT_EQ(m.dialog_id, 0);
    EXPECT_EQ(m.sender_id, 0);
    EXPECT_EQ(m.created_at_ms, 0);
    EXPECT_EQ(std::get<TextContent>(m.payload).text, "");
}

TEST(MessageJson, ThrowsWithoutTypeOrPayload) {
    EXPECT_THROW(nlohmann::json::parse(R"({"payload": {"text": "x"}})").get<Message>(), nlohmann::json::exception);
    EXPECT_THROW(nlohmann::json::parse(R"({"type": 0})").get<Message>(), nlohmann::json::exception);
}

TEST(MessageJson, ThrowsOnUnsupportedType) {
    EXPECT_THROW(nlohmann::json::parse(R"({"type": 4, "payload": {}})").get<Message>(), std::runtime_error);
    EXPECT_THROW(nlohmann::json::parse(R"({"type": 99, "payload": {}})").get<Message>(), std::runtime_error);
}

TEST(UserJson, RoundTrip) {
    const User u{.username = "ivan", .user_id = 7};
    const nlohmann::json j = u;

    EXPECT_EQ(j, nlohmann::json({{"user_id", 7}, {"username", "ivan"}}));

    const auto restored = j.get<User>();
    EXPECT_EQ(restored.username, "ivan");
    EXPECT_EQ(restored.user_id, 7);
}

TEST(UserJson, ThrowsWhenFieldMissing) {
    EXPECT_THROW(nlohmann::json::parse(R"({"username": "ivan"})").get<User>(), nlohmann::json::exception);
}

TEST(MetaDialogJson, RoundTripKeepsTransferredFields) {
    MetaDialog_Client md;
    md.dialog_id = kBigId;
    md.peer_id = 3;
    md.username = "masha";
    md.last_msg_preview = "Привет";
    md.last_msg_timestamp = kTimestampMs;

    const nlohmann::json j = md;
    const auto restored = j.get<MetaDialog_Client>();

    EXPECT_EQ(restored.dialog_id, kBigId);
    EXPECT_EQ(restored.peer_id, 3);
    EXPECT_EQ(restored.username, "masha");
    EXPECT_EQ(restored.last_msg_preview, "Привет");
    EXPECT_EQ(restored.last_msg_timestamp, kTimestampMs);
}

TEST(MetaDialogJson, FieldsNotInJsonAreZeroInitialized) {
    const auto j = nlohmann::json::parse(
        R"({"dialog_id": 1, "peer_id": 2, "username": "u", "last_msg_preview": "", "last_msg_timestamp": 0})");
    const auto md = j.get<MetaDialog_Client>();

    EXPECT_EQ(md.unread_count, 0);
    EXPECT_EQ(md.display_name, "");
}
