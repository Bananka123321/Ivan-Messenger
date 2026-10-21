#include <gtest/gtest.h>

#include "PacketIO_Client.h"

namespace {

QByteArray frame(const std::string& payload) {
    const uint32_t len = qToBigEndian(static_cast<uint32_t>(payload.size()));
    QByteArray result(reinterpret_cast<const char*>(&len), sizeof(len));
    result.append(payload.data(), static_cast<qsizetype>(payload.size()));
    return result;
}

QByteArray header(const uint32_t len) {
    const uint32_t net = qToBigEndian(len);
    return {reinterpret_cast<const char*>(&net), sizeof(net)};
}

}

TEST(PacketIOClient, WaitsForFullHeader) {
    QByteArray buffer = frame("abc").left(3);

    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), std::nullopt);
    EXPECT_EQ(buffer.size(), 3);
}

TEST(PacketIOClient, WaitsForFullPayload) {
    QByteArray buffer = frame("hello").left(6);

    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), std::nullopt);
    EXPECT_EQ(buffer.size(), 6) << "неполный пакет должен остаться в буфере";
}

TEST(PacketIOClient, ExtractsCompletePacketAndConsumesIt) {
    QByteArray buffer = frame(R"({"type":"ping"})");

    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), R"({"type":"ping"})");
    EXPECT_TRUE(buffer.isEmpty());
}

TEST(PacketIOClient, ExtractsSeveralPacketsFromOneBuffer) {
    QByteArray buffer = frame("one") + frame("two") + frame("three").left(5);

    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), "one");
    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), "two");
    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), std::nullopt);
    EXPECT_EQ(buffer, frame("three").left(5));
}

TEST(PacketIOClient, AssemblesPacketArrivingByteByByte) {
    const QByteArray wire = frame("Привет");
    QByteArray buffer;

    for (qsizetype i = 0; i < wire.size() - 1; ++i) {
        buffer.append(wire[i]);
        ASSERT_EQ(PacketIO_Client::extractPacket(buffer), std::nullopt) << "байт " << i;
    }
    buffer.append(wire.back());

    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), "Привет");
}

TEST(PacketIOClient, PreservesBinaryData) {
    const std::string payload("a\0b\xFF", 4);
    QByteArray buffer = frame(payload);

    EXPECT_EQ(PacketIO_Client::extractPacket(buffer), payload);
}

TEST(PacketIOClient, AcceptsMaximumSize) {
    QByteArray buffer = frame(std::string(MAX_PACKET_SIZE, 'x'));

    const auto packet = PacketIO_Client::extractPacket(buffer);

    ASSERT_TRUE(packet.has_value());
    EXPECT_EQ(packet->size(), MAX_PACKET_SIZE);
}

TEST(PacketIOClient, DropsBufferOnInvalidLength) {
    for (const uint32_t len : {0u, MAX_PACKET_SIZE + 1}) {
        QByteArray buffer = header(len) + frame("next");

        EXPECT_EQ(PacketIO_Client::extractPacket(buffer), std::nullopt) << "len=" << len;
        EXPECT_TRUE(buffer.isEmpty()) << "len=" << len;
    }
}
