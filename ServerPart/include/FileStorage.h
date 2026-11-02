#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

// Физическое хранение файлов на диске.
// Файлы лежат в root под случайными именами (storage_name), исходное имя хранится только в БД,
// поэтому имя от клиента никогда не попадает в путь на диске.
class FileStorage {
public:
    explicit FileStorage(std::filesystem::path root);

    // 32 hex-символа из криптостойкого генератора OpenSSL
    static std::string generateName();

    // Пишет data начиная с offset. Всё, что было в файле после offset, отбрасывается,
    // поэтому повторная запись того же чанка после обрыва безопасна.
    bool write(const std::string& name, uint64_t offset, std::string_view data);

    // Читает до length байт начиная с offset. nullopt, если файла нет или offset за концом.
    std::optional<std::string> read(const std::string& name, uint64_t offset, size_t length) const;

    // SHA-256 первых size байт файла в hex (нижний регистр)
    std::optional<std::string> sha256(const std::string& name, uint64_t size) const;

    std::optional<uint64_t> size(const std::string& name) const;

    void remove(const std::string& name);

    static std::string sha256Hex(std::string_view data);

    [[nodiscard]] const std::filesystem::path& root() const { return root_; }

private:
    std::filesystem::path root_;

    // nullopt, если имя не похоже на сгенерированное (защита от выхода за пределы root)
    std::optional<std::filesystem::path> pathFor(const std::string& name) const;
};
