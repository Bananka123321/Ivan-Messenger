#include <gtest/gtest.h>

#include <fstream>

#include "FileStorage.h"
#include "TempDir.h"

class FileStorageTest : public testing::Test {
protected:
    TempDir dir_;
    FileStorage storage_{dir_.path()};
    const std::string name_ = FileStorage::generateName();
};

TEST(FileStorageStatic, GeneratesUniqueHexNames) {
    const auto a = FileStorage::generateName();
    const auto b = FileStorage::generateName();

    EXPECT_EQ(a.size(), 32u);
    EXPECT_NE(a, b);
    EXPECT_EQ(a.find_first_not_of("0123456789abcdef"), std::string::npos);
}

TEST(FileStorageStatic, Sha256OfKnownVectors) {
    EXPECT_EQ(FileStorage::sha256Hex(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    EXPECT_EQ(FileStorage::sha256Hex("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST_F(FileStorageTest, WritesChunksSequentiallyAndReadsBack) {
    ASSERT_TRUE(storage_.write(name_, 0, "hello "));
    ASSERT_TRUE(storage_.write(name_, 6, "world"));

    EXPECT_EQ(storage_.size(name_), 11u);
    EXPECT_EQ(storage_.read(name_, 0, 100), "hello world");
    EXPECT_EQ(storage_.read(name_, 6, 3), "wor");
    EXPECT_EQ(storage_.read(name_, 11, 10), "");
    EXPECT_EQ(storage_.read(name_, 12, 10), std::nullopt);
}

TEST_F(FileStorageTest, RewritingChunkTruncatesTail) {
    ASSERT_TRUE(storage_.write(name_, 0, "aaaa"));
    ASSERT_TRUE(storage_.write(name_, 4, "bbbbbbbb"));

    // Клиент не получил подтверждение второго чанка и прислал его заново, уже другого размера
    ASSERT_TRUE(storage_.write(name_, 4, "cc"));

    EXPECT_EQ(storage_.read(name_, 0, 100), "aaaacc");
}

TEST_F(FileStorageTest, RejectsGapsAndForeignNames) {
    EXPECT_FALSE(storage_.write(name_, 10, "x"));        // файла нет, а offset не 0
    ASSERT_TRUE(storage_.write(name_, 0, "abc"));
    EXPECT_FALSE(storage_.write(name_, 5, "x"));         // дыра между 3 и 5

    EXPECT_FALSE(storage_.write("../../etc/passwd", 0, "x"));
    EXPECT_FALSE(storage_.write("not-hex-name-not-hex-name-000000", 0, "x"));
    EXPECT_EQ(storage_.read("../secret", 0, 10), std::nullopt);
}

TEST_F(FileStorageTest, HashesOnlyRequestedPrefix) {
    ASSERT_TRUE(storage_.write(name_, 0, "abcdef"));

    EXPECT_EQ(storage_.sha256(name_, 3), FileStorage::sha256Hex("abc"));
    EXPECT_EQ(storage_.sha256(name_, 6), FileStorage::sha256Hex("abcdef"));
    EXPECT_EQ(storage_.sha256(name_, 7), std::nullopt);
}

TEST_F(FileStorageTest, HashesLargeBinaryFile) {
    std::string data;
    for (int i = 0; i < 300'000; ++i)
        data += static_cast<char>(i * 31);

    for (size_t offset = 0; offset < data.size(); offset += 65'536)
        ASSERT_TRUE(storage_.write(name_, offset, std::string_view(data).substr(offset, 65'536)));

    EXPECT_EQ(storage_.sha256(name_, data.size()), FileStorage::sha256Hex(data));
}

TEST_F(FileStorageTest, RemoveDeletesFile) {
    ASSERT_TRUE(storage_.write(name_, 0, "x"));
    storage_.remove(name_);

    EXPECT_EQ(storage_.size(name_), std::nullopt);
    EXPECT_FALSE(std::filesystem::exists(dir_.path() / name_));
}
