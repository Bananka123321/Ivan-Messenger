#include <gtest/gtest.h>

#include <chrono>

#include "ClientSession.h"
#include "TlsPair.h"

namespace {

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

}

TEST(ClientSession, StartsUnauthenticated) {
    const int64_t before = nowMs();
    const ClientSession session(5, nullptr);

    EXPECT_EQ(session.getSocket(), 5);
    EXPECT_EQ(session.getUserId(), 0);
    EXPECT_EQ(session.getUsername(), "");
    EXPECT_FALSE(session.getIsAuthenticated());
    EXPECT_FALSE(session.getConnected());
    EXPECT_EQ(session.getTempToken(), "");
    EXPECT_GE(session.getLastActivity(), before);
    EXPECT_LE(session.getLastActivity(), nowMs());
}

TEST(ClientSession, StoresUserAndState) {
    ClientSession session(5, nullptr);

    session.setUser(7, "ivan");
    session.setIsAuthenticated(true);
    session.setConnected(true);
    session.setTempToken("token");
    session.setLastActivity(123);

    EXPECT_EQ(session.getUserId(), 7);
    EXPECT_EQ(session.getUsername(), "ivan");
    EXPECT_TRUE(session.getIsAuthenticated());
    EXPECT_TRUE(session.getConnected());
    EXPECT_EQ(session.getTempToken(), "token");
    EXPECT_EQ(session.getLastActivity(), 123);
}

TEST(ClientSession, SendDeliversPacketToClient) {
    TlsPair tls;
    const ClientSession session(-1, tls.takeServer());

    ASSERT_TRUE(session.send(R"({"type":"error"})"));

    EXPECT_EQ(tls.clientReadPackets(), std::vector<std::string>{R"({"type":"error"})"});
}

TEST(ClientSession, ReceiveReadsPacketFromClient) {
    TlsPair tls;
    const ClientSession session(-1, tls.takeServer());
    ASSERT_TRUE(tls.clientSendPacket(R"({"type":"ping"})"));

    std::string message;
    ASSERT_TRUE(session.receive(message));
    EXPECT_EQ(message, R"({"type":"ping"})");
}
