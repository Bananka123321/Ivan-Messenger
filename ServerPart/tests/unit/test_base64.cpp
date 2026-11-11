#include <gtest/gtest.h>

#include "Base64.h"

TEST(Base64, EncodesRfc4648Vectors) {
    EXPECT_EQ(base64::encode(""), "");
    EXPECT_EQ(base64::encode("f"), "Zg==");
    EXPECT_EQ(base64::encode("fo"), "Zm8=");
    EXPECT_EQ(base64::encode("foo"), "Zm9v");
    EXPECT_EQ(base64::encode("foob"), "Zm9vYg==");
    EXPECT_EQ(base64::encode("fooba"), "Zm9vYmE=");
    EXPECT_EQ(base64::encode("foobar"), "Zm9vYmFy");
}

TEST(Base64, DecodesRfc4648Vectors) {
    EXPECT_EQ(base64::decode(""), "");
    EXPECT_EQ(base64::decode("Zg=="), "f");
    EXPECT_EQ(base64::decode("Zm8="), "fo");
    EXPECT_EQ(base64::decode("Zm9v"), "foo");
    EXPECT_EQ(base64::decode("Zm9vYmFy"), "foobar");
}

TEST(Base64, RoundTripsEveryByteValue) {
    std::string bytes;
    for (int i = 0; i < 256; ++i)
        bytes += static_cast<char>(i);

    for (size_t len = 0; len <= bytes.size(); len += 37) {
        const auto part = bytes.substr(0, len);
        EXPECT_EQ(base64::decode(base64::encode(part)), part) << "len=" << len;
    }
}

TEST(Base64, RejectsMalformedInput) {
    EXPECT_EQ(base64::decode("Zm9"), std::nullopt);       // длина не кратна 4
    EXPECT_EQ(base64::decode("Zm9v!A=="), std::nullopt);  // чужой символ
    EXPECT_EQ(base64::decode("Z=9v"), std::nullopt);      // '=' в середине
    EXPECT_EQ(base64::decode("Zg==Zm9v"), std::nullopt);  // padding не в последнем блоке
    EXPECT_EQ(base64::decode("Z==="), std::nullopt);      // слишком много '='
}
