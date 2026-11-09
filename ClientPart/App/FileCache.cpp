#include "FileCache.h"
#include "Logging.h"

#include <QAtomicInteger>
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>

namespace {

constexpr int kSchemaVersion = 1;

QString nextConnectionName() {
    static QAtomicInteger<int> counter{0};
    return QStringLiteral("file_cache_%1").arg(counter.fetchAndAddRelaxed(1));
}

CachedFile fromQuery(const QSqlQuery& q) {
    CachedFile f;
    f.fileId = q.value("file_id").toLongLong();
    f.filename = q.value("filename").toString();
    f.mimeType = q.value("mime_type").toString();
    f.kind = q.value("kind").toInt();
    f.sizeBytes = q.value("size_bytes").toULongLong();
    f.sha256 = q.value("sha256").toString();
    f.localPath = q.value("local_path").toString();
    f.downloadedBytes = q.value("downloaded_bytes").toULongLong();
    f.completed = q.value("completed").toBool();
    return f;
}

}

FileCache::FileCache(const QString& dbPath) : connectionName_(nextConnectionName()) {
    QDir().mkpath(QFileInfo(dbPath).absolutePath());

    auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName_);
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        qCCritical(logFiles) << "Не удалось открыть кэш файлов" << dbPath << db.lastError().text();
        return;
    }

    open_ = migrate();
    if (open_)
        qCInfo(logFiles) << "Кэш файлов открыт:" << dbPath;
}

FileCache::~FileCache() {
    {
        auto db = QSqlDatabase::database(connectionName_, false);
        if (db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(connectionName_);
}

bool FileCache::isOpen() const {
    return open_;
}

QString FileCache::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/file_cache.sqlite");
}

bool FileCache::migrate() {
    auto db = QSqlDatabase::database(connectionName_);
    QSqlQuery q(db);

    if (!q.exec(QStringLiteral("PRAGMA user_version")) || !q.next()) {
        qCCritical(logFiles) << "Не удалось прочитать версию схемы кэша" << q.lastError().text();
        return false;
    }
    const int version = q.value(0).toInt();
    if (version >= kSchemaVersion)
        return true;

    // Версия 1: таблица файлов
    const bool ok = q.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS files ("
        " file_id INTEGER PRIMARY KEY,"
        " filename TEXT NOT NULL,"
        " mime_type TEXT NOT NULL,"
        " kind INTEGER NOT NULL,"
        " size_bytes INTEGER NOT NULL,"
        " sha256 TEXT NOT NULL,"
        " local_path TEXT NOT NULL DEFAULT '',"
        " downloaded_bytes INTEGER NOT NULL DEFAULT 0,"
        " completed INTEGER NOT NULL DEFAULT 0,"
        " updated_at INTEGER NOT NULL DEFAULT (strftime('%s', 'now'))"
        ")"))
        && q.exec(QStringLiteral("PRAGMA user_version = %1").arg(kSchemaVersion));

    if (!ok)
        qCCritical(logFiles) << "Ошибка миграции кэша файлов" << q.lastError().text();
    return ok;
}

std::optional<CachedFile> FileCache::get(const qint64 fileId) const {
    if (!open_)
        return std::nullopt;

    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral("SELECT * FROM files WHERE file_id = ?"));
    q.addBindValue(fileId);
    if (!q.exec() || !q.next())
        return std::nullopt;
    return fromQuery(q);
}

bool FileCache::upsert(const CachedFile& file) {
    if (!open_)
        return false;

    // Если на сервере это уже другой файл (изменились размер или хэш), прогресс начинается заново
    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral(
        "INSERT INTO files (file_id, filename, mime_type, kind, size_bytes, sha256, local_path, downloaded_bytes, completed)"
        " VALUES (:id, :name, :mime, :kind, :size, :sha, :path, :done_bytes, :completed)"
        " ON CONFLICT(file_id) DO UPDATE SET"
        "  filename = excluded.filename,"
        "  mime_type = excluded.mime_type,"
        "  kind = excluded.kind,"
        "  local_path = excluded.local_path,"
        "  downloaded_bytes = CASE WHEN files.size_bytes = excluded.size_bytes AND files.sha256 = excluded.sha256"
        "                          AND files.local_path = excluded.local_path"
        "                     THEN max(files.downloaded_bytes, excluded.downloaded_bytes) ELSE excluded.downloaded_bytes END,"
        "  completed = CASE WHEN files.size_bytes = excluded.size_bytes AND files.sha256 = excluded.sha256"
        "                   AND files.local_path = excluded.local_path"
        "              THEN max(files.completed, excluded.completed) ELSE excluded.completed END,"
        "  size_bytes = excluded.size_bytes,"
        "  sha256 = excluded.sha256,"
        "  updated_at = strftime('%s', 'now')"));
    q.bindValue(QStringLiteral(":id"), file.fileId);
    q.bindValue(QStringLiteral(":name"), file.filename);
    q.bindValue(QStringLiteral(":mime"), file.mimeType);
    q.bindValue(QStringLiteral(":kind"), file.kind);
    q.bindValue(QStringLiteral(":size"), static_cast<qint64>(file.sizeBytes));
    q.bindValue(QStringLiteral(":sha"), file.sha256);
    q.bindValue(QStringLiteral(":path"), file.localPath);
    q.bindValue(QStringLiteral(":done_bytes"), static_cast<qint64>(file.downloadedBytes));
    q.bindValue(QStringLiteral(":completed"), file.completed ? 1 : 0);

    if (!q.exec()) {
        qCWarning(logFiles) << "Не удалось сохранить файл в кэш" << file.fileId << q.lastError().text();
        return false;
    }
    return true;
}

bool FileCache::updateProgress(const qint64 fileId, const quint64 downloadedBytes) {
    if (!open_)
        return false;

    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral(
        "UPDATE files SET downloaded_bytes = ?, updated_at = strftime('%s', 'now')"
        " WHERE file_id = ? AND ? <= size_bytes"));
    q.addBindValue(static_cast<qint64>(downloadedBytes));
    q.addBindValue(fileId);
    q.addBindValue(static_cast<qint64>(downloadedBytes));
    return q.exec() && q.numRowsAffected() == 1;
}

bool FileCache::markCompleted(const qint64 fileId) {
    if (!open_)
        return false;

    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral(
        "UPDATE files SET completed = 1, downloaded_bytes = size_bytes, updated_at = strftime('%s', 'now')"
        " WHERE file_id = ?"));
    q.addBindValue(fileId);
    return q.exec() && q.numRowsAffected() == 1;
}

bool FileCache::remove(const qint64 fileId) {
    if (!open_)
        return false;

    QSqlQuery q(QSqlDatabase::database(connectionName_));
    q.prepare(QStringLiteral("DELETE FROM files WHERE file_id = ?"));
    q.addBindValue(fileId);
    return q.exec();
}

std::optional<QString> FileCache::completedPath(const qint64 fileId) {
    const auto file = get(fileId);
    if (!file || !file->completed)
        return std::nullopt;

    const QFileInfo info(file->localPath);
    if (info.exists() && static_cast<quint64>(info.size()) == file->sizeBytes)
        return file->localPath;

    qCInfo(logFiles) << "Файл из кэша пропал или изменён, будет скачан заново:" << fileId << file->localPath;
    remove(fileId);
    return std::nullopt;
}
