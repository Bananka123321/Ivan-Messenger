#include <gtest/gtest.h>

#include <thread>

#include "DB_ConnectionPool.h"
#include "TestDatabase.h"

class DBConnectionPoolTest : public DatabaseTest {};

TEST_F(DBConnectionPoolTest, OpensOneConnectionUpFront) {
    DB_ConnectionPool pool(TestDatabase::connectionString(), 3);

    EXPECT_EQ(pool.size(), 1u);
    EXPECT_EQ(pool.idle(), 1u);
}

TEST_F(DBConnectionPoolTest, ReusesReturnedConnection) {
    DB_ConnectionPool pool(TestDatabase::connectionString(), 3);

    const pqxx::connection* first;
    {
        const auto lease = pool.acquire();
        first = &*lease;
    }
    const auto lease = pool.acquire();

    EXPECT_EQ(&*lease, first);
    EXPECT_EQ(pool.size(), 1u);
}

TEST_F(DBConnectionPoolTest, GrowsUpToLimitForParallelUsers) {
    DB_ConnectionPool pool(TestDatabase::connectionString(), 3);

    {
        auto a = pool.acquire();
        auto b = pool.acquire();
        auto c = pool.acquire();
        EXPECT_NE(&*a, &*b);
        EXPECT_NE(&*b, &*c);
        EXPECT_EQ(pool.size(), 3u);
        EXPECT_EQ(pool.idle(), 0u);
    }

    EXPECT_EQ(pool.idle(), 3u);
}

TEST_F(DBConnectionPoolTest, WaitsForFreeConnectionWhenExhausted) {
    DB_ConnectionPool pool(TestDatabase::connectionString(), 1);
    auto held = std::make_unique<DB_ConnectionPool::Lease>(pool.acquire());

    std::atomic<bool> acquired{false};
    std::thread waiter([&] {
        const auto lease = pool.acquire();
        acquired = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(acquired.load());

    held.reset();
    waiter.join();
    EXPECT_TRUE(acquired.load());
    EXPECT_EQ(pool.size(), 1u);
}

TEST_F(DBConnectionPoolTest, ReplacesClosedConnection) {
    DB_ConnectionPool pool(TestDatabase::connectionString(), 2);
    {
        auto lease = pool.acquire();
        lease->close();
    }

    EXPECT_EQ(pool.size(), 0u);

    auto lease = pool.acquire();
    pqxx::work txn(*lease);
    EXPECT_EQ(txn.query_value<int>("SELECT 1"), 1);
    EXPECT_EQ(pool.size(), 1u);
}

TEST_F(DBConnectionPoolTest, ParallelTransactionsDoNotInterfere) {
    DB_ConnectionPool pool(TestDatabase::connectionString(), 4);
    constexpr int kThreads = 8;
    constexpr int kQueries = 25;

    std::atomic<int> ok{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            for (int i = 0; i < kQueries; ++i) {
                auto lease = pool.acquire();
                pqxx::work txn(*lease);
                if (txn.query_value<int>("SELECT $1::int", pqxx::params(t * 1000 + i)) == t * 1000 + i)
                    ++ok;
                txn.commit();
            }
        });
    }
    for (auto& t : threads)
        t.join();

    EXPECT_EQ(ok.load(), kThreads * kQueries);
    EXPECT_LE(pool.size(), 4u);
}
