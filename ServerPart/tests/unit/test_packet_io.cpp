#include <gtest/gtest.h>

#include "PacketIO_Server.h"
#include "TlsPair.h"

namespace {

std::string header(const uint32_t len) {
    const uint32_t net = htonl(len);
    return {reinterpret_cast<const char*>(&net), sizeof(net)};
}

}

TEST(PacketIOServer, SendWritesBigEndianLengthThenPayload) {
    TlsPair tls;

    ASSERT_TRUE(PacketIO_Server::sendPacket(tls.server(), "abc"));

    std::string raw;
    while (tls.clientHasData())
        raw += tls.clientReadRaw(64);
    EXPECT_EQ(raw, header(3) + "abc");
}

TEST(PacketIOServer, RoundTripServerToClient) {
    TlsPair tls;

    ASSERT_TRUE(PacketIO_Server::sendPacket(tls.server(), R"({"type":"ping"})"));

    const auto packets = tls.clientReadPackets();
    ASSERT_EQ(packets.size(), 1u);
    EXPECT_EQ(packets[0], R"({"type":"ping"})");
}

TEST(PacketIOServer, RoundTripClientToServer) {
    TlsPair tls;
    ASSERT_TRUE(tls.clientSendPacket("hello"));

    std::string received;
    ASSERT_TRUE(PacketIO_Server::recvPacket(tls.server(), received));
    EXPECT_EQ(received, "hello");
}

TEST(PacketIOServer, KeepsOrderOfSeveralPackets) {
    TlsPair tls;
    for (const auto* p : {"one", "two", "three"})
        ASSERT_TRUE(PacketIO_Server::sendPacket(tls.server(), p));

    EXPECT_EQ(tls.clientReadPackets(), (std::vector<std::string>{"one", "two", "three"}));
}

TEST(PacketIOServer, PreservesBinaryAndUtf8Data) {
    TlsPair tls;
    const std::string payload = std::string("a\0b\xFF", 4) + "Привет 👋";

    ASSERT_TRUE(tls.clientSendPacket(payload));

    std::string received;
    ASSERT_TRUE(PacketIO_Server::recvPacket(tls.server(), received));
    EXPECT_EQ(received, payload);
}

TEST(PacketIOServer, AcceptsPacketOfMaximumSize) {
    TlsPair tls;
    const std::string payload(MAX_PACKET_SIZE, 'x');

    ASSERT_TRUE(tls.clientSendPacket(payload));

    std::string received;
    ASSERT_TRUE(PacketIO_Server::recvPacket(tls.server(), received));
    EXPECT_EQ(received.size(), MAX_PACKET_SIZE);
}

TEST(PacketIOServer, RejectsPacketLargerThanMaximum) {
    TlsPair tls;
    tls.clientWriteRaw(header(MAX_PACKET_SIZE + 1) + "x");

    std::string received;
    EXPECT_FALSE(PacketIO_Server::recvPacket(tls.server(), received));
}

TEST(PacketIOServer, RejectsZeroLengthPacket) {
    TlsPair tls;
    tls.clientWriteRaw(header(0));

    std::string received;
    EXPECT_FALSE(PacketIO_Server::recvPacket(tls.server(), received));
}

TEST(PacketIOServer, FailsOnIncompletePacket) {
    TlsPair tls;
    tls.clientWriteRaw(header(10) + "12345");

    std::string received;
    EXPECT_FALSE(PacketIO_Server::recvPacket(tls.server(), received));
}

TEST(PacketIOServer, FailsWhenPeerClosedConnection) {
    TlsPair tls;
    SSL_shutdown(tls.client());

    std::string received;
    EXPECT_FALSE(PacketIO_Server::recvPacket(tls.server(), received));
}

TEST(PacketIOServer, FramePrefixesBigEndianLength) {
    EXPECT_EQ(PacketIO_Server::frame("abc"), header(3) + "abc");
}

TEST(PacketIOServer, ExtractWaitsForCompleteHeaderAndPayload) {
    std::string buffer = header(5).substr(0, 2);
    std::string packet;

    EXPECT_EQ(PacketIO_Server::extractPacket(buffer, packet), PacketIO_Server::Extract::Incomplete);

    buffer = header(5) + "abc";
    EXPECT_EQ(PacketIO_Server::extractPacket(buffer, packet), PacketIO_Server::Extract::Incomplete);
    EXPECT_EQ(buffer, header(5) + "abc");

    buffer += "de";
    EXPECT_EQ(PacketIO_Server::extractPacket(buffer, packet), PacketIO_Server::Extract::Packet);
    EXPECT_EQ(packet, "abcde");
    EXPECT_TRUE(buffer.empty());
}

TEST(PacketIOServer, ExtractTakesPacketsOneByOneAndKeepsTail) {
    std::string buffer = PacketIO_Server::frame("one") + PacketIO_Server::frame("two") + header(10) + "x";
    std::string packet;

    ASSERT_EQ(PacketIO_Server::extractPacket(buffer, packet), PacketIO_Server::Extract::Packet);
    EXPECT_EQ(packet, "one");
    ASSERT_EQ(PacketIO_Server::extractPacket(buffer, packet), PacketIO_Server::Extract::Packet);
    EXPECT_EQ(packet, "two");
    EXPECT_EQ(PacketIO_Server::extractPacket(buffer, packet), PacketIO_Server::Extract::Incomplete);
    EXPECT_EQ(buffer, header(10) + "x");
}

TEST(PacketIOServer, ExtractRejectsInvalidSizes) {
    std::string packet;

    std::string zero = header(0);
    EXPECT_EQ(PacketIO_Server::extractPacket(zero, packet), PacketIO_Server::Extract::Invalid);

    std::string tooBig = header(MAX_PACKET_SIZE + 1);
    EXPECT_EQ(PacketIO_Server::extractPacket(tooBig, packet), PacketIO_Server::Extract::Invalid);
}
