#pragma once

#include <condition_variable>
#include <memory>
#include <mutex>
#include <pqxx/pqxx>
#include <string>
#include <vector>

class DB_ConnectionPool {
public:
    class Lease {
    public:
        Lease(DB_ConnectionPool& pool, std::unique_ptr<pqxx::connection> conn);
        ~Lease();

        Lease(Lease&& other) noexcept;
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        Lease& operator=(Lease&&) = delete;

        pqxx::connection& operator*() const { return *conn_; }
        pqxx::connection* operator->() const { return conn_.get(); }

    private:
        DB_ConnectionPool* pool_;
        std::unique_ptr<pqxx::connection> conn_;
    };

    DB_ConnectionPool(std::string conn_str, size_t max_size);

    DB_ConnectionPool(const DB_ConnectionPool&) = delete;
    DB_ConnectionPool& operator=(const DB_ConnectionPool&) = delete;

    Lease acquire();

    [[nodiscard]] size_t size();
    [[nodiscard]] size_t idle();

private:
    void release(std::unique_ptr<pqxx::connection> conn);

    std::string conn_str_;
    size_t max_size_;
    size_t created_ = 0;
    std::vector<std::unique_ptr<pqxx::connection>> idle_;
    std::mutex mutex_;
    std::condition_variable available_;
};
