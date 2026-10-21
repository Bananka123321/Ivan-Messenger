#include <gtest/gtest.h>

#include <thread>

#include "HandlerTest.h"

namespace {

constexpr int kSenders = 8;
constexpr int kMessagesPerSender = 40;

}

class HandlerConcurrencyTest : public HandlerTest {
protected:
    void SetUp() override {
        HandlerTest::SetUp();
        if (IsSkipped() || HasFailure())
            return;
        hubId_ = signUp(hub_, "hubuser");
        for (int i = 0; i < kSenders; ++i) {
            auto& sender = senders_[i];
            senderIds_[i] = signUp(sender, "sender" + std::to_string(i));
            dialogIds_[i] = openDialog(sender, hubId_);
        }
    }

    int messageCount() { return exec("SELECT COUNT(*) FROM messages")[0][0].as<int>(); }

    TestClient hub_;
    int hubId_ = 0;
    std::array<TestClient, kSenders> senders_;
    std::array<int, kSenders> senderIds_{};
    std::array<int64_t, kSenders> dialogIds_{};
};

TEST_F(HandlerConcurrencyTest, ParallelSendersDeliverEveryMessageIntact) {
    std::vector<std::thread> threads;
    for (int i = 0; i < kSenders; ++i) {
        threads.emplace_back([this, i] {
            for (int n = 0; n < kMessagesPerSender; ++n)
                send(senders_[i], textMessage(dialogIds_[i], senderIds_[i], "msg " + std::to_string(n)));
        });
    }
    for (auto& t : threads)
        t.join();

    for (auto& sender : senders_)
        EXPECT_EQ(sender.receive(), std::vector<nlohmann::json>{});

    const auto delivered = hub_.receive();
    ASSERT_EQ(delivered.size(), static_cast<size_t>(kSenders * kMessagesPerSender));

    std::map<int, int> perSender;
    for (const auto& packet : delivered) {
        ASSERT_EQ(packet["type"], "sendMessage");
        ++perSender[packet["data"]["sender_id"].get<int>()];
    }
    for (const int id : senderIds_)
        EXPECT_EQ(perSender[id], kMessagesPerSender);

    EXPECT_EQ(messageCount(), kSenders * kMessagesPerSender);
}
