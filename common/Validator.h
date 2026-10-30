#pragma once
#include <string>
#include <optional>
#include <string_view>

#include "nlohmann/json.hpp"
#include "Message.h"

namespace Validator {
    std::optional<std::string> username(std::string_view value);
    std::optional<std::string> password(std::string_view value);
    std::optional<std::string> message(std::string_view value);
    std::optional<std::string> search(std::string_view value);

    bool valid_string_field(const nlohmann::json& j, const std::string& key, std::optional<std::string> (*Validator)(std::string_view), std::string& error);
    bool valid_int_field(const nlohmann::json& j, const std::string& key, const long int& minV, const long int& maxV, std::string& error);
    std::string sanitize(std::string_view value);
    std::optional<std::string> validateMessage(const Message& msg);

    std::optional<std::string> filename(std::string_view value);
    std::optional<std::string> mimeType(std::string_view value);
    std::optional<std::string> sha256Hex(std::string_view value);
    bool isFileKind(MessageType type);
}
