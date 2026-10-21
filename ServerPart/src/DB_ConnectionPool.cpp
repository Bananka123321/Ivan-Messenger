#include "../include/DB_ConnectionPool.h"
#include "../include/Logger.h"

DB_ConnectionPool::Lease::Lease(DB_ConnectionPool& pool, std::unique_ptr<pqxx::connection> conn)
    : pool_(&pool), conn_(std::move(conn)) {}

DB_ConnectionPool::Lease::Lease(Lease&& other) noexcept
    : pool_(other.pool_), conn_(std::move(other.conn_)) {}

DB_ConnectionPool::Lease::~Lease() {
    if (conn_) {
        pool_->release(std::move(conn_));
    }
}

DB_ConnectionPool::DB_ConnectionPool(std::string conn_str, const size_t max_size)
    : conn_str_(std::move(conn_str)), max_size_(std::max<size_t>(max_size, 1)) {
    idle_.push_back(std::make_unique<pqxx::connection>(conn_str_));
    created_ = 1;
    LOG_INFO(DB, "Инициализация пула соединений с БД. максимум=", max_size_);
}

DB_ConnectionPool::Lease DB_ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(mutex_);
    available_.wait(lock, [this] { return !idle_.empty() || created_ < max_size_; });

    if (!idle_.empty()) {
        auto conn = std::move(idle_.back());
        idle_.pop_back();
        return {*this, std::move(conn)};
    }

    ++created_;
    lock.unlock();

    try {
        auto conn = std::make_unique<pqxx::connection>(conn_str_);
        LOG_DEBUG(DB, "Открыто новое соединение с БД");
        return {*this, std::move(conn)};
    } catch (...) {
        lock.lock();
        --created_;
        available_.notify_one();
        throw;
    }
}

size_t DB_ConnectionPool::size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return created_;
}

size_t DB_ConnectionPool::idle() {
    std::lock_guard<std::mutex> lock(mutex_);
    return idle_.size();
}

void DB_ConnectionPool::release(std::unique_ptr<pqxx::connection> conn) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (conn->is_open()) {
            idle_.push_back(std::move(conn));
        } else {
            LOG_WARNING(DB, "Соединение с БД разорвано, оно будет пересоздано");
            --created_;
        }
    }
    available_.notify_one();
}
