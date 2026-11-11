#include "Validator.h"
#include "protocol.h"

std::optional<std::string> Validator::username(const std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() < 4) return "Must be minimum 4 characters\n";
    if (value.length() > 32) return "Maximum 32 characters\n";
    
    for (const char c : value) {
        if (const bool ok = (c >= 'a' && c <='z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <='9') || c == '_' || c == '-'; !ok) return "Only letters, numbers, underscore and dash allowed\n";
    }
    
    return std::nullopt;
}

std::optional<std::string> Validator::password(const std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() < 6) return "Must be minimum 6 characters\n";
    if (value.length() > 32) return "Maximum 32 characters\n";
    
    return std::nullopt;
}

std::optional<std::string> Validator::message(const std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() > 4096) return "Too long(max 4096 characters)\n";
    
    return std::nullopt;
}

std::optional<std::string> Validator::search(const std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() > 64) return "Too long(max 64 characters)\n";
    
    return std::nullopt;
}

bool Validator::valid_string_field(const nlohmann::json& j, const std::string& key, std::optional<std::string> (*Validator)(std::string_view), std::string& error){
    if (!j.contains(key)) {
        error = "Invalid key";
        return false;
    }

    if (const auto err = Validator(j[key].get<std::string_view>())) {
        error = *err;
        return false;
    }

    return true;
    
}

bool Validator::valid_int_field(const nlohmann::json& j, const std::string& key, const long int& minV, const long int& maxV, std::string& error){
    if (!j.contains(key)) {
        error = "Invalid key";
        return false;
    }

    if (j[key] < minV || j[key] > maxV) {
        error = "value out of range [" + std::to_string(minV) + ", " + std::to_string(maxV) + "]\n";
        return false;
    }

    return true;
    
}

std::optional<std::string> Validator::validateMessage(const Message& msg) {
    return std::visit([]<typename T0>(const T0& content) -> std::optional<std::string> {
        using T = std::decay_t<T0>;
        
        if constexpr (std::is_same_v<T, TextContent>) {
            return Validator::message(content.text);
        }
        else if constexpr (std::is_same_v<T, MediaContent>) {
            if (content.file_id <= 0) return "Media file_id is missing";
            if (content.size_bytes > protocol::FILE_MAX_SIZE) return "File too large (max 100MB)";
            if (content.caption.size() > 1024) return "Caption too long (max 1024 characters)";
            return std::nullopt;
        }
        else if constexpr (std::is_same_v<T, VoiceContent>) {
            if (content.file_id <= 0) return "Voice file_id is missing";
            if (content.duration_sec > 300) return "Voice message too long (max 5 min)";
            return std::nullopt;
        }

        return std::nullopt;
    }, msg.payload);
}

std::string Validator::sanitize(const std::string_view value) {
    std::string result;
    result.reserve(value.length());

    size_t i = 0;
    while (i < value.length()) {
        const auto c = static_cast<unsigned char>(value[i]);

        int seq_len = 0;
        uint32_t codepoint = 0;

        if (c < 0x80) {
            seq_len = 1;
            codepoint = c;
        } else if ((c & 0xE0) == 0xC0) {
            seq_len = 2;
            codepoint = c & 0x1F;
        } else if ((c & 0xF0) == 0xE0) {
            seq_len = 3;
            codepoint = c & 0x0F;
        } else if ((c & 0xF8) == 0xF0) {
            seq_len = 4;
            codepoint = c & 0x07;
        } else {
            i++;
            continue;
        }

        if (i + seq_len > value.length()) {
            break;
        }

        bool valid = true;
        for (int j = 1; j < seq_len; j++) {
            const auto cont = static_cast<unsigned char>(value[i + j]);
            if ((cont & 0xC0) != 0x80) {
                valid = false;
                break;
            }
            codepoint = (codepoint << 6) | (cont & 0x3F);
        }

        if (!valid) {
            i++;
            continue;
        }

        bool allowed = false;

        if (codepoint >= 0x20 && codepoint <= 0x7E) {
            allowed = true;
        }

        else if (codepoint >= 0x0400 && codepoint <= 0x04FF) {
            allowed = true;
        }

        else if (codepoint >= 0x0500 && codepoint <= 0x052F) {
            allowed = true;
        }

        else if (codepoint == '\n' || codepoint == '\t' || codepoint == '\r') {
            allowed = true;
        }

        else if (codepoint >= 0x1F600 && codepoint <= 0x1F64F) {
            allowed = true;
        }
        else if (codepoint >= 0x1F300 && codepoint <= 0x1F5FF) {
            allowed = true;
        }
        else if (codepoint >= 0x1F680 && codepoint <= 0x1F6FF) {
            allowed = true;
        }
        else if (codepoint >= 0x2600 && codepoint <= 0x26FF) {
            allowed = true;
        }
        else if (codepoint >= 0x2700 && codepoint <= 0x27BF) {
            allowed = true;
        }

        if (allowed) {
            for (int j = 0; j < seq_len; j++) {
                result += value[i + j];
            }
        }

        i += seq_len;
    }

    if (result.length() > 4096)
        result = result.substr(0, 4096);

    return result;
}

std::optional<std::string> Validator::filename(const std::string_view value) {
    if (value.empty()) return "Filename is empty";
    if (value.size() > 255) return "Filename too long (max 255 bytes)";
    if (value == "." || value == "..") return "Invalid filename";

    for (const char c : value) {
        if (c == '/' || c == '\\') return "Filename must not contain path separators";
        if (static_cast<unsigned char>(c) < 0x20 || c == 0x7F) return "Filename contains control characters";
    }

    return std::nullopt;
}

std::optional<std::string> Validator::mimeType(const std::string_view value) {
    if (value.empty()) return "MIME type is empty";
    if (value.size() > 127) return "MIME type too long";

    const auto slash = value.find('/');
    if (slash == std::string_view::npos || slash == 0 || slash + 1 == value.size())
        return "Invalid MIME type";

    for (size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                        c == '.' || c == '+' || c == '-' || c == '_' || (c == '/' && i == slash);
        if (!ok) return "Invalid MIME type";
    }

    return std::nullopt;
}

std::optional<std::string> Validator::sha256Hex(const std::string_view value) {
    if (value.size() != 64) return "SHA-256 must be 64 hex characters";

    for (const char c : value) {
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
            return "SHA-256 must be lowercase hex";
    }

    return std::nullopt;
}

bool Validator::isFileKind(const MessageType type) {
    return type == MessageType::Image || type == MessageType::File || type == MessageType::Voice;
}
