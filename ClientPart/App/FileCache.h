#pragma once

#include <QString>
#include <optional>

// Запись локального кэша о файле с сервера
struct CachedFile {
    qint64 fileId = 0;
    QString filename;
    QString mimeType;
    int kind = 0;                   // MessageType
    quint64 sizeBytes = 0;
    QString sha256;
    QString localPath;              // куда файл скачивается / где лежит готовый
    quint64 downloadedBytes = 0;    // для докачки после обрыва или перезапуска
    bool completed = false;
};

// Локальный кэш файлов на SQLite.
// Позволяет не скачивать файл повторно и продолжать незаконченное скачивание с нужного места.
class FileCache {
public:
    explicit FileCache(const QString& dbPath = defaultPath());
    ~FileCache();

    FileCache(const FileCache&) = delete;
    FileCache& operator=(const FileCache&) = delete;

    [[nodiscard]] bool isOpen() const;

    [[nodiscard]] std::optional<CachedFile> get(qint64 fileId) const;

    // Создаёт запись или обновляет метаданные. Прогресс и признак готовности не сбрасываются,
    // если метаданные (размер и хэш) не изменились.
    bool upsert(const CachedFile& file);

    bool updateProgress(qint64 fileId, quint64 downloadedBytes);
    bool markCompleted(qint64 fileId);
    bool remove(qint64 fileId);

    // Путь к готовому файлу, если он всё ещё на диске и его размер совпадает.
    // Если файл удалили или испортили, запись сбрасывается и файл нужно скачать заново.
    std::optional<QString> completedPath(qint64 fileId);

    static QString defaultPath();

private:
    QString connectionName_;
    bool open_ = false;

    bool migrate();
};
