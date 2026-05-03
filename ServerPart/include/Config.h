#pragma once

#include <string>
#include <optional>
#include <nlohmann/json.hpp>

struct DBConfig {
    std::string name;
    std::string user;
    std::string password;
    std::string host;
    int port;
    int timeout;

    std::string getConnectionStr() const;
};

struct ServerConfig {
    int port;
    int max_connections;
};

struct Config {
public:
    static std::optional<std::string> load(const std::string& path = "config.json");

    static DBConfig db_config;
    static ServerConfig server_config;
    static bool loaded;

    static const DBConfig& getDB();
    static const ServerConfig& getServer();
};