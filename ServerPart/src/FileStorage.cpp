#include "../include/FileStorage.h"
#include "../include/Logger.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <memory>

#include <openssl/evp.h>
#include <openssl/rand.h>

namespace {

std::string toHex(const unsigned char* data, const size_t size) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (size_t i = 0; i < size; ++i) {
        out += digits[data[i] >> 4];
        out += digits[data[i] & 0x0F];
    }
    return out;
}

using DigestCtx = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

}

FileStorage::FileStorage(std::filesystem::path root) : root_(std::move(root)) {
    std::error_code ec;
    std::filesystem::create_directories(root_, ec);
    if (ec)
        LOG_CRITICAL(FILES, "Не удалось создать каталог хранилища ", root_.string(), ": ", ec.message());
    else
        LOG_INFO(FILES, "Файловое хранилище: ", std::filesystem::absolute(root_).string());
}

std::string FileStorage::generateName() {
    std::array<unsigned char, 16> bytes{};
    if (RAND_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1)
        throw std::runtime_error("RAND_bytes failed");
    return toHex(bytes.data(), bytes.size());
}

std::optional<std::filesystem::path> FileStorage::pathFor(const std::string& name) const {
    if (name.size() != 32)
        return std::nullopt;
    for (const char c : name)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return std::nullopt;
    return root_ / name;
}

bool FileStorage::write(const std::string& name, const uint64_t offset, const std::string_view data) {
    const auto path = pathFor(name);
    if (!path) {
        LOG_WARNING(FILES, "Некорректное имя файла в хранилище: '", name, "'");
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(*path, ec)) {
        if (offset != 0) {
            LOG_WARNING(FILES, "Запись с offset=", offset, " в несуществующий файл ", name);
            return false;
        }
        std::ofstream create(*path, std::ios::binary);
        if (!create) {
            LOG_CRITICAL(FILES, "Не удалось создать файл ", path->string());
            return false;
        }
    }

    const auto current = std::filesystem::file_size(*path, ec);
    if (ec || current < offset) {
        LOG_WARNING(FILES, "Дыра в файле ", name, ": размер=", current, " offset=", offset);
        return false;
    }
    if (current > offset) {
        std::filesystem::resize_file(*path, offset, ec);
        if (ec) {
            LOG_CRITICAL(FILES, "Не удалось обрезать файл ", name, ": ", ec.message());
            return false;
        }
    }

    std::ofstream out(*path, std::ios::binary | std::ios::in | std::ios::out);
    out.seekp(static_cast<std::streamoff>(offset));
    out.write(data.data(), static_cast<std::streamsize>(data.size()));
    out.flush();

    if (!out) {
        LOG_CRITICAL(FILES, "Ошибка записи в файл ", name, " offset=", offset, " size=", data.size());
        return false;
    }
    return true;
}

std::optional<std::string> FileStorage::read(const std::string& name, const uint64_t offset, const size_t length) const {
    const auto path = pathFor(name);
    if (!path)
        return std::nullopt;

    std::ifstream in(*path, std::ios::binary);
    if (!in)
        return std::nullopt;

    std::error_code ec;
    const auto total = std::filesystem::file_size(*path, ec);
    if (ec || offset > total)
        return std::nullopt;

    const auto toRead = static_cast<size_t>(std::min<uint64_t>(length, total - offset));
    std::string buffer(toRead, '\0');
    in.seekg(static_cast<std::streamoff>(offset));
    in.read(buffer.data(), static_cast<std::streamsize>(toRead));
    if (static_cast<size_t>(in.gcount()) != toRead)
        return std::nullopt;

    return buffer;
}

std::optional<std::string> FileStorage::sha256(const std::string& name, const uint64_t size) const {
    const auto path = pathFor(name);
    if (!path)
        return std::nullopt;

    std::ifstream in(*path, std::ios::binary);
    if (!in)
        return std::nullopt;

    const DigestCtx ctx(EVP_MD_CTX_new(), &EVP_MD_CTX_free);
    if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1)
        return std::nullopt;

    std::array<char, 64 * 1024> buffer{};
    uint64_t left = size;
    while (left > 0) {
        const auto chunk = static_cast<std::streamsize>(std::min<uint64_t>(left, buffer.size()));
        in.read(buffer.data(), chunk);
        const auto got = in.gcount();
        if (got <= 0)
            return std::nullopt;  // файл короче, чем ожидалось
        EVP_DigestUpdate(ctx.get(), buffer.data(), static_cast<size_t>(got));
        left -= static_cast<uint64_t>(got);
    }

    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLen = 0;
    if (EVP_DigestFinal_ex(ctx.get(), digest.data(), &digestLen) != 1)
        return std::nullopt;

    return toHex(digest.data(), digestLen);
}

std::optional<uint64_t> FileStorage::size(const std::string& name) const {
    const auto path = pathFor(name);
    if (!path)
        return std::nullopt;
    std::error_code ec;
    const auto s = std::filesystem::file_size(*path, ec);
    if (ec)
        return std::nullopt;
    return s;
}

void FileStorage::remove(const std::string& name) {
    if (const auto path = pathFor(name)) {
        std::error_code ec;
        std::filesystem::remove(*path, ec);
        if (ec)
            LOG_WARNING(FILES, "Не удалось удалить файл ", name, ": ", ec.message());
    }
}

std::string FileStorage::sha256Hex(const std::string_view data) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> digest{};
    unsigned int digestLen = 0;
    EVP_Digest(data.data(), data.size(), digest.data(), &digestLen, EVP_sha256(), nullptr);
    return toHex(digest.data(), digestLen);
}
