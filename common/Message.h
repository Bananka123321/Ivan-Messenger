#pragma once

#include <string>
#include <variant>
#include <cstdint>

struct TextContent {
    std::string text;
};

struct MediaContent {
    std::string url;
    std::string caption;
    std::string filename;
    uint64_t size_bytes = 0;
    int64_t file_id = 0;        // id из таблицы files, по нему файл скачивается
    std::string mime_type;
};

struct VoiceContent {
    std::string url;
    uint32_t duration_sec = 0;
    int64_t file_id = 0;
    uint64_t size_bytes = 0;
};

using MessagePayload = std::variant<TextContent, MediaContent, VoiceContent>;

enum class MessageType : uint8_t {
    Text,
    Image,
    Voice,
    File,
    Sticker
};

struct Message {
    int64_t id = 0;
    int dialog_id = 0;
    int sender_id = 0;
    MessageType type = MessageType::Text;
    int64_t created_at_ms = 0;

    MessagePayload payload;

    [[nodiscard]] std::string getPreview() const {
        return std::visit([]<typename T0>(const T0& content) -> std::string {
            using T = std::decay_t<T0>;

            if constexpr (std::is_same_v<T, TextContent>) {
                return content.text;
            } else if constexpr (std::is_same_v<T, MediaContent>) {
                return content.filename.empty() ? "Файл" : "Файл: " + content.filename;
            } else if constexpr (std::is_same_v<T, VoiceContent>) {
                return "Голосовое сообщение, " + std::to_string(content.duration_sec) + " сек";
            }
            return "Неизвестное сообщение";
        }, payload);
    }
};