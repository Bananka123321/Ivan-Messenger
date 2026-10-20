#include <gtest/gtest.h>

#include <thread>

#include "SessionManager.h"

namespace {

std::shared_ptr<ClientSession> makeSession(const int userId) {
    auto s = std::make_shared<ClientSession>(-1, nullptr);
    s->setUser(userId, "user" + std::to_string(userId));
    return s;
}

}

TEST(SessionManager, FindsAddedSessionByUserId) {
    SessionManager sm;
    const auto a = makeSession(1);
    const auto b = makeSession(2);
    sm.add(a);
    sm.add(b);

    EXPECT_EQ(sm.getByUserId(1), a);
    EXPECT_EQ(sm.getByUserId(2), b);
    EXPECT_EQ(sm.getByUserId(3), nullptr);
}

TEST(SessionManager, IgnoresNullptr) {
    SessionManager sm;
    sm.add(nullptr);
    sm.remove(nullptr);

    EXPECT_TRUE(sm.getAll().empty());
}

TEST(SessionManager, RemovesSession) {
    SessionManager sm;
    const auto a = makeSession(1);
    sm.add(a);

    sm.remove(a);

    EXPECT_EQ(sm.getByUserId(1), nullptr);
    EXPECT_TRUE(sm.getAll().empty());
}

TEST(SessionManager, RemovingUnknownSessionKeepsOthers) {
    SessionManager sm;
    const auto a = makeSession(1);
    sm.add(a);

    sm.remove(makeSession(1));

    EXPECT_EQ(sm.getByUserId(1), a);
}

TEST(SessionManager, GetAllReturnsSnapshot) {
    SessionManager sm;
    sm.add(makeSession(1));
    auto all = sm.getAll();
    sm.add(makeSession(2));

    EXPECT_EQ(all.size(), 1u);
    EXPECT_EQ(sm.getAll().size(), 2u);
}

TEST(SessionManager, ReturnsFirstSessionWhenUserHasSeveral) {
    SessionManager sm;
    const auto phone = makeSession(1);
    const auto desktop = makeSession(1);
    sm.add(phone);
    sm.add(desktop);

    EXPECT_EQ(sm.getByUserId(1), phone);

    sm.remove(phone);
    EXPECT_EQ(sm.getByUserId(1), desktop);
}

TEST(SessionManager, UpdateActivitySetsCurrentTime) {
    const auto s = makeSession(1);
    s->setLastActivity(0);

    SessionManager::updateActivity(s);

    EXPECT_GT(s->getLastActivity(), 1'700'000'000'000);
    SessionManager::updateActivity(nullptr);
}

TEST(SessionManager, IsSafeForConcurrentAccess) {
    SessionManager sm;
    constexpr int kThreads = 8;
    constexpr int kPerThread = 200;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&sm, t] {
            for (int i = 0; i < kPerThread; ++i) {
                const auto s = makeSession(t * kPerThread + i);
                sm.add(s);
                (void)sm.getByUserId(s->getUserId());
                if (i % 2 == 0)
                    sm.remove(s);
            }
        });
    }
    for (auto& th : threads)
        th.join();

    EXPECT_EQ(sm.getAll().size(), static_cast<size_t>(kThreads * kPerThread / 2));
}
