#include "Config.h"
#include <fstream>
#include <iostream>

std::string DBConfig::getConnectionStr() const {
    return "dbname=" + name + " user=" + user + " password=" + password + " host=" + host + " port=" + std::to_string(port) + " connect_timeout=" + std::to_string(timeout);
}

DBConfig Config::db_config;
ServerConfig Config::server_config;
bool Config::loaded = false;

std::optional<std::string> Config::load(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) return "Config file not found: " + path;

        nlohmann::json j;
        file >> j;

        if (!j.contains("database") || !j["database"].is_object())
            return "Missing or invalid 'database' section in config";

        const auto& db = j["database"];
        
        if (!db.contains("name") || !db.contains("user") || !db.contains("password") || !db.contains("host"))
            return "Database config missing required fields";

        db_config.name = db["name"].get<std::string>();
        db_config.user = db["user"].get<std::string>();
        db_config.password = db["password"].get<std::string>();
        db_config.host = db["host"].get<std::string>();
        db_config.port = db.value("port", 5432);
        db_config.timeout = db.value("timeout", 10); 
        
        if (j.contains("server") && j["server"].is_object()) {
            const auto& srv = j["server"];
            server_config.port = srv.value("port", 6767);
            server_config.max_connections = srv.value("max_connections", 100);
        } else {
            server_config.port = 6767;
            server_config.max_connections = 100;
        }
        
        loaded = true;
        return std::nullopt;
        
    } catch (const nlohmann::json::parse_error& e) {
        return "JSON parse error: " + std::string(e.what());
    } catch (const nlohmann::json::type_error& e) {
        return "JSON type error: " + std::string(e.what());
    } catch (const std::exception& e) {
        return "Config load error: " + std::string(e.what());
    }
}

const DBConfig& Config::getDB() {
    if (!loaded) {
        throw std::runtime_error("Config not loaded");
    }
    return db_config;
}

const ServerConfig& Config::getServer() {
    if (!loaded) {
        throw std::runtime_error("Config not loaded");
    }
    return server_config;
    
}