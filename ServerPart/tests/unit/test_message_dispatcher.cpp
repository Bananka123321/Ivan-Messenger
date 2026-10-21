#include <gtest/gtest.h>

#include "MessageDispatcher.h"
#include "TlsPair.h"

namespace {

struct Peer {
    TlsPair tls;
    std::shared_ptr<ClientSession> session;

    explicit Peer(const int userId) : session(std::make_shared<ClientSession>(-1, tls.takeServer())) {
        session->setUser(userId, "user" + std::to_string(userId));
    }
};

}

TEST(MessageDispatcher, SendToDeliversMessage) {
    SessionManager sm;
    const MessageDispatcher dispatcher(sm);
    Peer peer(1);

    dispatcher.sendTo(peer.session, "hello");
    ASSERT_TRUE(peer.session->flush());

    EXPECT_EQ(peer.tls.clientReadPackets(), std::vector<std::string>{"hello"});
}

TEST(MessageDispatcher, SendToNullptrDoesNothing) {
    SessionManager sm;
    MessageDispatcher dispatcher(sm);
    int disconnects = 0;
    dispatcher.onDisconnect = [&](const auto&) { ++disconnects; };

    dispatcher.sendTo(nullptr, "hello");

    EXPECT_EQ(disconnects, 0);
}

TEST(MessageDispatcher, SendFailureTriggersDisconnect) {
    SessionManager sm;
    MessageDispatcher dispatcher(sm);
    Peer peer(1);
    std::shared_ptr<ClientSession> disconnected;
    dispatcher.onDisconnect = [&](const auto& s) { disconnected = s; };

    peer.session->close();
    dispatcher.sendTo(peer.session, "hello");

    EXPECT_EQ(disconnected, peer.session);
}

TEST(MessageDispatcher, SendFailureWithoutCallbackDoesNotCrash) {
    SessionManager sm;
    const MessageDispatcher dispatcher(sm);
    Peer peer(1);

    peer.session->close();
    dispatcher.sendTo(peer.session, "hello");
}

TEST(MessageDispatcher, BroadcastReachesEverySession) {
    SessionManager sm;
    const MessageDispatcher dispatcher(sm);
    Peer a(1), b(2), c(3);
    sm.add(a.session);
    sm.add(b.session);
    sm.add(c.session);

    dispatcher.broadcast("news");

    for (Peer* p : {&a, &b, &c})
        ASSERT_TRUE(p->session->flush());
    for (Peer* p : {&a, &b, &c})
        EXPECT_EQ(p->tls.clientReadPackets(), std::vector<std::string>{"news"});
}

TEST(MessageDispatcher, BroadcastDisconnectsOnlyFailedSessions) {
    SessionManager sm;
    MessageDispatcher dispatcher(sm);
    Peer ok(1), broken(2);
    sm.add(ok.session);
    sm.add(broken.session);

    std::vector<std::shared_ptr<ClientSession>> disconnected;
    dispatcher.onDisconnect = [&](const auto& s) { disconnected.push_back(s); };

    broken.session->close();
    dispatcher.broadcast("news");

    ASSERT_TRUE(ok.session->flush());
    EXPECT_EQ(ok.tls.clientReadPackets(), std::vector<std::string>{"news"});
    ASSERT_EQ(disconnected.size(), 1u);
    EXPECT_EQ(disconnected[0], broken.session);
}
