#pragma once

#include <string>
#include <iostream>
#include <mutex>
#include <unordered_set>
#include <chrono>
#include <iomanip>
#include <sstream>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    CRITICAL = 3
};

enum class LogCategory {
    SESSION,
    NETWORK,
    AUTH,
    DB,
    HANDLER,
    FILES,
    MAIN
};

class Logger {
public:
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    void setMinLevel(const LogLevel level) {
        minLevel_ = level;
    }

    void enableCategory(const std::string& category) {
        enabledCategories_.insert(category);
    }

    void disableCategory(const std::string& category) {
        enabledCategories_.erase(category);
    }

    void enableAllCategories() {
        allCategoriesEnabled_ = true;
    }

    template<typename... Args>
    void log(const LogLevel level, const LogCategory category, Args&&... args) {
        if (level < minLevel_) return;

        const std::string catStr = categoryToString(category);
        if (!allCategoriesEnabled_ && !enabledCategories_.contains(catStr)) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);

        const auto now = std::chrono::system_clock::now();
        const auto time_t = std::chrono::system_clock::to_time_t(now);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::tm tm{};
        localtime_r(&time_t, &tm);
        
        const auto color = getColor(level);
        constexpr auto reset = "\033[0m";
        constexpr auto gray = "\033[90m";
        const auto levelText = getLevelText(level);
        
        std::cerr << gray << "["
                  << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
                  << "." << std::setfill('0') << std::setw(3) << ms.count() 
                  << "] " << reset
                  << color << std::left << std::setw(7) << levelText << reset << " "
                  << gray << "[" << catStr << "] " << reset;
        
        (std::cerr << ... << std::forward<Args>(args));
        std::cerr << std::endl;
    }

private:
    Logger() = default;
    
    std::mutex mutex_;
    LogLevel minLevel_ = LogLevel::DEBUG;
    std::unordered_set<std::string> enabledCategories_;
    bool allCategoriesEnabled_ = false;

    static std::string categoryToString(const LogCategory cat) {
        switch (cat) {
            case LogCategory::SESSION: return "server.session";
            case LogCategory::NETWORK: return "server.network";
            case LogCategory::AUTH: return "server.auth";
            case LogCategory::DB: return "server.db";
            case LogCategory::HANDLER: return "server.handler";
            case LogCategory::FILES: return "server.files";
            case LogCategory::MAIN: return "server.main";
            default: return "unknown";
        }
    }

    static const char* getColor(const LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "\033[36m";
            case LogLevel::INFO: return "\033[32m";
            case LogLevel::WARNING: return "\033[33m";
            case LogLevel::CRITICAL: return "\033[31m\033[1m";
            default: return "\033[0m";
        }
    }

    static const char* getLevelText(const LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARNING";
            case LogLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

#define LOG_DEBUG(cat, ...) Logger::instance().log(LogLevel::DEBUG, LogCategory::cat, __VA_ARGS__)
#define LOG_INFO(cat, ...) Logger::instance().log(LogLevel::INFO, LogCategory::cat, __VA_ARGS__)
#define LOG_WARNING(cat, ...) Logger::instance().log(LogLevel::WARNING, LogCategory::cat, __VA_ARGS__)
#define LOG_CRITICAL(cat, ...) Logger::instance().log(LogLevel::CRITICAL, LogCategory::cat, __VA_ARGS__)