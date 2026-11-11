#include <gtest/gtest.h>

#include "DB_DialogManager.h"
#include "DB_FileManager.h"
#include "TestDatabase.h"

class DBFilesTest : public DatabaseTest {
protected:
    void SetUp() override {
        DatabaseTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        files_ = std::make_unique<DB_FileManager>(pool());
        ivan_ = insertUser("ivan");
        masha_ = insertUser("masha");
        dialog_ = DB_DialogManager(pool()).findOrCreatePrivateDialog(ivan_, masha_).value();
    }

    FileRecord record(const std::string& sha = std::string(64, 'a'), const uint64_t size = 1000) const {
        FileRecord f;
        f.info.filename = "отчёт.pdf";
        f.info.mime_type = "application/pdf";
        f.info.kind = MessageType::File;
        f.info.size_bytes = size;
        f.info.sha256 = sha;
        f.owner_id = ivan_;
        f.dialog_id = dialog_;
        f.storage_name = std::string(32, 'f');
        return f;
    }

    std::unique_ptr<DB_FileManager> files_;
    int ivan_ = 0;
    int masha_ = 0;
    int64_t dialog_ = 0;
};

TEST_F(DBFilesTest, CreateAndGetKeepAllFields) {
    const auto id = files_->create(record());
    ASSERT_TRUE(id.has_value());

    const auto f = files_->get(*id);
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->info.file_id, *id);
    EXPECT_EQ(f->info.filename, "отчёт.pdf");
    EXPECT_EQ(f->info.mime_type, "application/pdf");
    EXPECT_EQ(f->info.kind, MessageType::File);
    EXPECT_EQ(f->info.size_bytes, 1000u);
    EXPECT_EQ(f->info.sha256, std::string(64, 'a'));
    EXPECT_EQ(f->owner_id, ivan_);
    EXPECT_EQ(f->dialog_id, dialog_);
    EXPECT_EQ(f->storage_name, std::string(32, 'f'));
    EXPECT_EQ(f->uploaded_bytes, 0u);
    EXPECT_EQ(f->status, FileStatus::Uploading);
}

TEST_F(DBFilesTest, GetUnknownFileReturnsNothing) {
    EXPECT_FALSE(files_->get(12345).has_value());
}

TEST_F(DBFilesTest, StorageNameIsUnique) {
    ASSERT_TRUE(files_->create(record()).has_value());
    EXPECT_EQ(files_->create(record()), std::nullopt);
}

TEST_F(DBFilesTest, ProgressCannotExceedSize) {
    const auto id = files_->create(record()).value();

    EXPECT_TRUE(files_->updateProgress(id, 500));
    EXPECT_EQ(files_->get(id)->uploaded_bytes, 500u);

    EXPECT_FALSE(files_->updateProgress(id, 1001));
    EXPECT_EQ(files_->get(id)->uploaded_bytes, 500u);
}

TEST_F(DBFilesTest, CompleteFileStopsAcceptingProgress) {
    const auto id = files_->create(record()).value();
    ASSERT_TRUE(files_->updateProgress(id, 1000));

    ASSERT_TRUE(files_->markComplete(id));

    EXPECT_EQ(files_->get(id)->status, FileStatus::Complete);
    EXPECT_FALSE(files_->updateProgress(id, 10));
    EXPECT_FALSE(exec("SELECT completed_at FROM files WHERE id = " + std::to_string(id))[0][0].is_null());
}

TEST_F(DBFilesTest, FailedStatusIsStored) {
    const auto id = files_->create(record()).value();
    ASSERT_TRUE(files_->markFailed(id));
    EXPECT_EQ(files_->get(id)->status, FileStatus::Failed);
}

TEST_F(DBFilesTest, FindResumableMatchesOnlyUnfinishedUploadOfSameFile) {
    const auto id = files_->create(record()).value();
    ASSERT_TRUE(files_->updateProgress(id, 300));

    const auto found = files_->findResumable(ivan_, dialog_, std::string(64, 'a'), 1000);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->info.file_id, id);
    EXPECT_EQ(found->uploaded_bytes, 300u);

    EXPECT_FALSE(files_->findResumable(masha_, dialog_, std::string(64, 'a'), 1000).has_value());
    EXPECT_FALSE(files_->findResumable(ivan_, dialog_, std::string(64, 'b'), 1000).has_value());
    EXPECT_FALSE(files_->findResumable(ivan_, dialog_, std::string(64, 'a'), 999).has_value());

    ASSERT_TRUE(files_->markComplete(id));
    EXPECT_FALSE(files_->findResumable(ivan_, dialog_, std::string(64, 'a'), 1000).has_value());
}

TEST_F(DBFilesTest, DeletingUserRemovesTheirFiles) {
    const auto id = files_->create(record()).value();

    exec("DELETE FROM users WHERE id = " + std::to_string(ivan_));

    EXPECT_FALSE(files_->get(id).has_value());
}
