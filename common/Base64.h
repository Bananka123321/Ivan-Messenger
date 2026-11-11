#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

// Base64 (RFC 4648) для передачи двоичных чанков файлов внутри JSON-пакетов.
// Header-only и без зависимостей, чтобы одинаково работать на сервере и на клиенте.
namespace base64 {

inline constexpr std::string_view kAlphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline std::string encode(std::string_view data) {
    std::string out;
    out.reserve((data.size() + 2) / 3 * 4);

    size_t i = 0;
    for (; i + 2 < data.size(); i += 3) {
        const uint32_t n = (static_cast<uint8_t>(data[i]) << 16) |
                           (static_cast<uint8_t>(data[i + 1]) << 8) |
                           static_cast<uint8_t>(data[i + 2]);
        out += kAlphabet[(n >> 18) & 0x3F];
        out += kAlphabet[(n >> 12) & 0x3F];
        out += kAlphabet[(n >> 6) & 0x3F];
        out += kAlphabet[n & 0x3F];
    }

    if (const size_t rest = data.size() - i; rest == 1) {
        const uint32_t n = static_cast<uint8_t>(data[i]) << 16;
        out += kAlphabet[(n >> 18) & 0x3F];
        out += kAlphabet[(n >> 12) & 0x3F];
        out += "==";
    } else if (rest == 2) {
        const uint32_t n = (static_cast<uint8_t>(data[i]) << 16) |
                           (static_cast<uint8_t>(data[i + 1]) << 8);
        out += kAlphabet[(n >> 18) & 0x3F];
        out += kAlphabet[(n >> 12) & 0x3F];
        out += kAlphabet[(n >> 6) & 0x3F];
        out += '=';
    }

    return out;
}

// Возвращает nullopt для некорректной строки: неверная длина, чужие символы, лишний padding.
inline std::optional<std::string> decode(std::string_view text) {
    static constexpr auto table = [] {
        std::array<int8_t, 256> t{};
        t.fill(-1);
        for (size_t i = 0; i < kAlphabet.size(); ++i)
            t[static_cast<uint8_t>(kAlphabet[i])] = static_cast<int8_t>(i);
        return t;
    }();

    if (text.size() % 4 != 0)
        return std::nullopt;

    size_t padding = 0;
    if (!text.empty() && text.back() == '=') ++padding;
    if (text.size() > 1 && text[text.size() - 2] == '=') ++padding;

    std::string out;
    out.reserve(text.size() / 4 * 3);

    for (size_t i = 0; i < text.size(); i += 4) {
        const bool last = i + 4 == text.size();
        uint32_t n = 0;
        for (size_t k = 0; k < 4; ++k) {
            const char c = text[i + k];
            if (c == '=') {
                // '=' допустим только в последних позициях последнего блока
                if (!last || k < 4 - padding)
                    return std::nullopt;
                n <<= 6;
                continue;
            }
            const int8_t v = table[static_cast<uint8_t>(c)];
            if (v < 0)
                return std::nullopt;
            n = (n << 6) | static_cast<uint32_t>(v);
        }

        out += static_cast<char>((n >> 16) & 0xFF);
        if (!last || padding < 2) out += static_cast<char>((n >> 8) & 0xFF);
        if (!last || padding < 1) out += static_cast<char>(n & 0xFF);
    }

    return out;
}

}
