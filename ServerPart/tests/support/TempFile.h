#pragma once

#include <filesystem>
#include <fstream>
#include <random>
#include <string>

class TempFile {
public:
    explicit TempFile(const std::string& content, const std::string& suffix = ".json") {
        std::random_device rd;
        path_ = std::filesystem::temp_directory_path() / ("messenger_test_" + std::to_string(rd()) + suffix);
        std::ofstream(path_) << content;
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path_, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    [[nodiscard]] std::string path() const { return path_.string(); }

private:
    std::filesystem::path path_;
};
