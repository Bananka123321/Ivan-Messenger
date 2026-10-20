#include <gtest/gtest.h>

#include "Validator.h"

class ValidUsername : public testing::TestWithParam<std::string> {};

TEST_P(ValidUsername, IsAccepted) {
    EXPECT_EQ(Validator::username(GetParam()), std::nullopt);
}

INSTANTIATE_TEST_SUITE_P(Validator, ValidUsername, testing::Values(
    "ivan",
    "Ivan_Petrov-2000",
    "user_name",
    "a-b-c-d",
    "0123",
    std::string(32, 'x')
));

class InvalidUsername : public testing::TestWithParam<std::string> {};

TEST_P(InvalidUsername, IsRejected) {
    EXPECT_NE(Validator::username(GetParam()), std::nullopt);
}

INSTANTIATE_TEST_SUITE_P(Validator, InvalidUsername, testing::Values(
    "",
    "abc",
    std::string(33, 'x'),
    "ivan petrov",
    "ivan@mail",
    "иван",
    "ivan.petrov",
    "ivan\n"
));

TEST(ValidatorUsername, ReportsSpecificReason) {
    EXPECT_EQ(Validator::username(""), "Cannot be empty\n");
    EXPECT_EQ(Validator::username("abc"), "Must be minimum 4 characters\n");
    EXPECT_EQ(Validator::username(std::string(33, 'a')), "Maximum 32 characters\n");
    EXPECT_EQ(Validator::username("bad name"), "Only letters, numbers, underscore and dash allowed\n");
}

TEST(ValidatorPassword, AcceptsLengthFrom6To32) {
    EXPECT_EQ(Validator::password("123456"), std::nullopt);
    EXPECT_EQ(Validator::password(std::string(32, 'p')), std::nullopt);
    EXPECT_EQ(Validator::password("p@ss w0rd!"), std::nullopt);
}

TEST(ValidatorPassword, RejectsEmptyShortAndLong) {
    EXPECT_EQ(Validator::password(""), "Cannot be empty\n");
    EXPECT_EQ(Validator::password("12345"), "Must be minimum 6 characters\n");
    EXPECT_EQ(Validator::password(std::string(33, 'p')), "Maximum 32 characters\n");
}

TEST(ValidatorPassword, CountsLengthInBytes) {
    EXPECT_EQ(Validator::password("пароль"), std::nullopt);
    EXPECT_NE(Validator::password("ааааааааааааааааа"), std::nullopt);
}

TEST(ValidatorMessage, AcceptsUpTo4096Bytes) {
    EXPECT_EQ(Validator::message("a"), std::nullopt);
    EXPECT_EQ(Validator::message(std::string(4096, 'a')), std::nullopt);
}

TEST(ValidatorMessage, RejectsEmptyAndTooLong) {
    EXPECT_NE(Validator::message(""), std::nullopt);
    EXPECT_NE(Validator::message(std::string(4097, 'a')), std::nullopt);
}

TEST(ValidatorSearch, AcceptsUpTo64Bytes) {
    EXPECT_EQ(Validator::search("iv"), std::nullopt);
    EXPECT_EQ(Validator::search(std::string(64, 's')), std::nullopt);
}

TEST(ValidatorSearch, RejectsEmptyAndTooLong) {
    EXPECT_NE(Validator::search(""), std::nullopt);
    EXPECT_NE(Validator::search(std::string(65, 's')), std::nullopt);
}

TEST(ValidatorStringField, FailsWhenKeyIsMissing) {
    const nlohmann::json j = {{"password", "secret1"}};
    std::string error;

    EXPECT_FALSE(Validator::valid_string_field(j, "username", Validator::username, error));
    EXPECT_EQ(error, "Invalid key");
}

TEST(ValidatorStringField, ForwardsValidatorError) {
    const nlohmann::json j = {{"username", "ab"}};
    std::string error;

    EXPECT_FALSE(Validator::valid_string_field(j, "username", Validator::username, error));
    EXPECT_EQ(error, "Must be minimum 4 characters\n");
}

TEST(ValidatorStringField, AcceptsValidValue) {
    const nlohmann::json j = {{"username", "ivan"}};
    std::string error;

    EXPECT_TRUE(Validator::valid_string_field(j, "username", Validator::username, error));
    EXPECT_TRUE(error.empty());
}

TEST(ValidatorStringField, ThrowsOnNonStringValue) {
    const nlohmann::json j = {{"username", 42}};
    std::string error;

    EXPECT_THROW(Validator::valid_string_field(j, "username", Validator::username, error), nlohmann::json::type_error);
}

TEST(ValidatorIntField, ChecksRangeInclusive) {
    const nlohmann::json j = {{"limit", 100}};
    std::string error;

    EXPECT_TRUE(Validator::valid_int_field(j, "limit", 1, 100, error));
    EXPECT_FALSE(Validator::valid_int_field(j, "limit", 1, 99, error));
    EXPECT_EQ(error, "value out of range [1, 99]\n");
}

TEST(ValidatorIntField, FailsWhenKeyIsMissing) {
    const nlohmann::json j = nlohmann::json::object();
    std::string error;

    EXPECT_FALSE(Validator::valid_int_field(j, "limit", 1, 100, error));
    EXPECT_EQ(error, "Invalid key");
}

TEST(ValidatorValidateMessage, ChecksTextContent) {
    Message msg;
    msg.payload = TextContent{.text = "Привет"};
    EXPECT_EQ(Validator::validateMessage(msg), std::nullopt);

    msg.payload = TextContent{.text = ""};
    EXPECT_NE(Validator::validateMessage(msg), std::nullopt);
}

TEST(ValidatorValidateMessage, ChecksMediaContent) {
    Message msg;
    msg.type = MessageType::File;

    msg.payload = MediaContent{.url = "files/1", .size_bytes = 100ULL * 1024 * 1024};
    EXPECT_EQ(Validator::validateMessage(msg), std::nullopt);

    msg.payload = MediaContent{.url = "", .size_bytes = 10};
    EXPECT_EQ(Validator::validateMessage(msg), "Media URL is empty");

    msg.payload = MediaContent{.url = "files/1", .size_bytes = 100ULL * 1024 * 1024 + 1};
    EXPECT_EQ(Validator::validateMessage(msg), "File too large (max 100MB)");
}

TEST(ValidatorValidateMessage, ChecksVoiceContent) {
    Message msg;
    msg.type = MessageType::Voice;

    msg.payload = VoiceContent{.url = "voice/1", .duration_sec = 300};
    EXPECT_EQ(Validator::validateMessage(msg), std::nullopt);

    msg.payload = VoiceContent{.url = "", .duration_sec = 5};
    EXPECT_EQ(Validator::validateMessage(msg), "Voice URL is empty");

    msg.payload = VoiceContent{.url = "voice/1", .duration_sec = 301};
    EXPECT_EQ(Validator::validateMessage(msg), "Voice message too long (max 5 min)");
}

TEST(ValidatorSanitize, KeepsPrintableAsciiAndCyrillic) {
    EXPECT_EQ(Validator::sanitize("Hello, World! 123"), "Hello, World! 123");
    EXPECT_EQ(Validator::sanitize("Привет, как дела?"), "Привет, как дела?");
    EXPECT_EQ(Validator::sanitize("Ёлка ёж"), "Ёлка ёж");
}

TEST(ValidatorSanitize, KeepsNewlinesAndTabs) {
    EXPECT_EQ(Validator::sanitize("a\nb\tc\r\n"), "a\nb\tc\r\n");
}

TEST(ValidatorSanitize, RemovesControlCharacters) {
    EXPECT_EQ(Validator::sanitize(std::string("a\x01" "b\x1B" "c\x7F", 6)), "abc");
    EXPECT_EQ(Validator::sanitize(std::string("a\0b", 3)), "ab");
}

TEST(ValidatorSanitize, RemovesInvalidUtf8) {
    EXPECT_EQ(Validator::sanitize("a\x80" "b"), "ab");
    EXPECT_EQ(Validator::sanitize("ok\xD0"), "ok");
    EXPECT_EQ(Validator::sanitize("\xD0" "z"), "z");
}

TEST(ValidatorSanitize, KeepsEmojiFromAllowedRanges) {
    EXPECT_EQ(Validator::sanitize("😀"), "😀");
    EXPECT_EQ(Validator::sanitize("🔥"), "🔥");
    EXPECT_EQ(Validator::sanitize("🚀"), "🚀");
    EXPECT_EQ(Validator::sanitize("❤"), "❤");
}

TEST(ValidatorSanitize, RemovesCharactersOutsideAllowedRanges) {
    EXPECT_EQ(Validator::sanitize("🥰"), "");
    EXPECT_EQ(Validator::sanitize("🤔"), "");
    EXPECT_EQ(Validator::sanitize("⭐"), "");
    EXPECT_EQ(Validator::sanitize("你好"), "");
    EXPECT_EQ(Validator::sanitize("café"), "caf");
}

TEST(ValidatorSanitize, ReturnsEmptyForEmptyInput) {
    EXPECT_EQ(Validator::sanitize(""), "");
}
