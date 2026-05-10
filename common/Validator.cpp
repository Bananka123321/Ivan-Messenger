#include "Validator.h"

std::optional<std::string> Validator::username(std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() < 4) return "Must be minimum 4 characters\n";
    if (value.length() > 32) return "Maximum 32 characters\n";
    
    for (char c : value) {
        bool ok = (c >= 'a' && c <='z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <='9') || c == '_' || c == '-';
        if (!ok) return "Only letters, numbers, underscore and dash allowed\n";
    }
    
    return std::nullopt;
}

std::optional<std::string> Validator::password(std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() < 6) return "Must be minimum 6 characters\n";
    if (value.length() > 32) return "Maximum 32 characters\n";
    
    return std::nullopt;
}

std::optional<std::string> Validator::message(std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() > 4096) return "Too long(max 4096 characters)\n";
    
    return std::nullopt;
}

std::optional<std::string> Validator::search(std::string_view value) {
    if (value.empty()) return "Cannot be empty\n";
    if (value.length() > 64) return "Too long(max 64 characters)\n";
    
    return std::nullopt;
}

bool Validator::valid_string_field(const nlohmann::json& j, const std::string& key, std::optional<std::string> (*Validator)(std::string_view), std::string& error){
    if (!j.contains(key)) {
        error = "Invalid key";
        return false;
    }

    auto err = Validator(j[key].get<std::string_view>());
    if (err) {
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