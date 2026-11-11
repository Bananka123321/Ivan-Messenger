#include <gtest/gtest.h>

#include <QFile>
#include <QTemporaryDir>

#include "FileCache.h"

namespace {

CachedFile sample(const QString& localPath, const qint64 id = 42) {
    CachedFile f;
    f.fileId = id;
    f.filename = QStringLiteral("отчёт.pdf");
    f.mimeType = QStringLiteral("application/pdf");
    f.kind = 3;
    f.sizeBytes = 5;
    f.sha256 = QString(64, QLatin1Char('a'));
    f.localPath = localPath;
    return f;
}

void writeFile(const QString& path, const QByteArray& data) {
    QFile file(path);
    ASSERT_TRUE(file.open(QIODevice::WriteOnly));
    file.write(data);
}

}

class FileCacheTest : public testing::Test {
protected:
    QTemporaryDir dir_;
    QString dbPath() const { return dir_.filePath(QStringLiteral("cache/file_cache.sqlite")); }
    QString localPath() const { return dir_.filePath(QStringLiteral("report.pdf")); }
};

TEST_F(FileCacheTest, CreatesDatabaseInMissingDirectory) {
    const FileCache cache(dbPath());

    EXPECT_TRUE(cache.isOpen());
    EXPECT_TRUE(QFile::exists(dbPath()));
}

TEST_F(FileCacheTest, StoresAndReadsAllFields) {
    FileCache cache(dbPath());
    ASSERT_TRUE(cache.upsert(sample(localPath())));

    const auto f = cache.get(42);
    ASSERT_TRUE(f.has_value());
    EXPECT_EQ(f->filename, QStringLiteral("отчёт.pdf"));
    EXPECT_EQ(f->mimeType, QStringLiteral("application/pdf"));
    EXPECT_EQ(f->kind, 3);
    EXPECT_EQ(f->sizeBytes, 5u);
    EXPECT_EQ(f->sha256, QString(64, QLatin1Char('a')));
    EXPECT_EQ(f->localPath, localPath());
    EXPECT_EQ(f->downloadedBytes, 0u);
    EXPECT_FALSE(f->completed);

    EXPECT_FALSE(cache.get(7).has_value());
}

TEST_F(FileCacheTest, KeepsProgressAcrossRestart) {
    {
        FileCache cache(dbPath());
        ASSERT_TRUE(cache.upsert(sample(localPath())));
        ASSERT_TRUE(cache.updateProgress(42, 3));
    }

    FileCache reopened(dbPath());
    EXPECT_EQ(reopened.get(42)->downloadedBytes, 3u);
}

TEST_F(FileCacheTest, UpsertWithSameMetadataKeepsProgress) {
    FileCache cache(dbPath());
    ASSERT_TRUE(cache.upsert(sample(localPath())));
    ASSERT_TRUE(cache.updateProgress(42, 3));

    ASSERT_TRUE(cache.upsert(sample(localPath())));

    EXPECT_EQ(cache.get(42)->downloadedBytes, 3u);
}

TEST_F(FileCacheTest, UpsertWithChangedFileResetsProgress) {
    FileCache cache(dbPath());
    ASSERT_TRUE(cache.upsert(sample(localPath())));
    ASSERT_TRUE(cache.updateProgress(42, 3));

    auto changed = sample(localPath());
    changed.sha256 = QString(64, QLatin1Char('b'));
    ASSERT_TRUE(cache.upsert(changed));

    EXPECT_EQ(cache.get(42)->downloadedBytes, 0u);
    EXPECT_EQ(cache.get(42)->sha256, QString(64, QLatin1Char('b')));
}

TEST_F(FileCacheTest, ProgressCannotExceedSize) {
    FileCache cache(dbPath());
    ASSERT_TRUE(cache.upsert(sample(localPath())));

    EXPECT_FALSE(cache.updateProgress(42, 6));
    EXPECT_FALSE(cache.updateProgress(7, 1));
    EXPECT_EQ(cache.get(42)->downloadedBytes, 0u);
}

TEST_F(FileCacheTest, CompletedPathRequiresFileOnDiskWithRightSize) {
    FileCache cache(dbPath());
    ASSERT_TRUE(cache.upsert(sample(localPath())));
    EXPECT_FALSE(cache.completedPath(42).has_value());   // ещё не скачан

    writeFile(localPath(), "12345");
    ASSERT_TRUE(cache.markCompleted(42));
    EXPECT_EQ(cache.get(42)->downloadedBytes, 5u);
    EXPECT_EQ(cache.completedPath(42), localPath());
}

TEST_F(FileCacheTest, ForgetsFileThatWasDeletedOrChanged) {
    FileCache cache(dbPath());
    ASSERT_TRUE(cache.upsert(sample(localPath())));
    writeFile(localPath(), "123");      // размер не совпадает с size_bytes
    ASSERT_TRUE(cache.markCompleted(42));

    EXPECT_FALSE(cache.completedPath(42).has_value());
    EXPECT_FALSE(cache.get(42).has_value());
}

TEST_F(FileCacheTest, TwoInstancesUseSeparateConnections) {
    FileCache a(dbPath());
    FileCache b(dir_.filePath(QStringLiteral("other.sqlite")));

    ASSERT_TRUE(a.upsert(sample(localPath(), 1)));
    ASSERT_TRUE(b.upsert(sample(localPath(), 2)));

    EXPECT_TRUE(a.get(1).has_value());
    EXPECT_FALSE(a.get(2).has_value());
    EXPECT_TRUE(b.get(2).has_value());
}
