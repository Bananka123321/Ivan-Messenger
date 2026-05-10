#pragma once
#include <string>
#include <optional>
#include <limits>
#include <optional>
#include <string_view>

#include "../common/nlohmann/json.hpp"

namespace Validator {
    std::optional<std::string> username(std::string_view value);
    std::optional<std::string> password(std::string_view value);
    std::optional<std::string> message(std::string_view value);
    std::optional<std::string> search(std::string_view value);

    bool valid_string_field(const nlohmann::json& j, const std::string& key, std::optional<std::string> (*Validator)(std::string_view), std::string& error);
    bool valid_int_field(const nlohmann::json& j, const std::string& key, const long int& minV, const long int& maxV, std::string& error);
}