#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <functional>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "ClientSession.h"
#include "TlsPair.h"

namespace {

int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

}

TEST(ClientSession, StartsUnauthenticated) {
    const int64_t before = nowMs();
    const ClientSession session(-1, nullptr);

    EXPECT_EQ(session.getSocket(), -1);
    EXPECT_EQ(session.getUserId(), 0);
    EXPECT_EQ(session.getUsername(), "");
    EXPECT_FALSE(session.getIsAuthenticated());
    EXPECT_FALSE(session.getConnected());
    EXPECT_EQ(session.getTempToken(), "");
    EXPECT_GE(session.getLastActivity(), before);
    EXPECT_LE(session.getLastActivity(), nowMs());
}

TEST(ClientSession, StoresUserAndState) {
    ClientSession session(-1, nullptr);

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
    ClientSession session(-1, tls.takeServer());

    ASSERT_TRUE(session.send(R"({"type":"error"})"));
    ASSERT_TRUE(session.flush());

    EXPECT_EQ(tls.clientReadPackets(), std::vector<std::string>{R"({"type":"error"})"});
}

TEST(ClientSession, ReceiveReadsPacketFromClient) {
    TlsPair tls;
    ClientSession session(-1, tls.takeServer());
    ASSERT_TRUE(tls.clientSendPacket(R"({"type":"ping"})"));

    std::string message;
    ASSERT_TRUE(session.receive(message));
    EXPECT_EQ(message, R"({"type":"ping"})");
}

namespace {

class SocketTls {
public:
    SocketTls() {
        signal(SIGPIPE, SIG_IGN);

        serverCtx_ = SSL_CTX_new(TLS_server_method());
        clientCtx_ = SSL_CTX_new(TLS_client_method());
        SSL_CTX_use_certificate_file(serverCtx_, TEST_CERT_FILE, SSL_FILETYPE_PEM);
        SSL_CTX_use_PrivateKey_file(serverCtx_, TEST_KEY_FILE, SSL_FILETYPE_PEM);
        SSL_CTX_set_verify(clientCtx_, SSL_VERIFY_NONE, nullptr);

        int fds[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
            throw std::runtime_error("socketpair");
        fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL, 0) | O_NONBLOCK);
        clientFd_ = fds[1];

        SSL* ssl = SSL_new(serverCtx_);
        SSL_set_fd(ssl, fds[0]);
        session = std::make_shared<ClientSession>(fds[0], ssl);

        client_ = SSL_new(clientCtx_);
        SSL_set_fd(client_, clientFd_);
    }

    ~SocketTls() {
        session.reset();
        clientDisconnect();
        SSL_CTX_free(serverCtx_);
        SSL_CTX_free(clientCtx_);
    }

    bool clientConnect() const { return SSL_connect(client_) == 1; }
    bool clientSend(const std::string& data) const { return PacketIO_Server::sendPacket(client_, data); }

    bool clientWriteRaw(const std::string& bytes) const {
        return SSL_write(client_, bytes.data(), static_cast<int>(bytes.size())) == static_cast<int>(bytes.size());
    }

    void clientDisconnect() {
        if (client_)
            SSL_free(client_);
        if (clientFd_ >= 0)
            close(clientFd_);
        client_ = nullptr;
        clientFd_ = -1;
    }

    std::string clientReceive() const {
        std::string data;
        PacketIO_Server::recvPacket(client_, data);
        return data;
    }

    std::shared_ptr<ClientSession> session;

private:
    SSL_CTX* serverCtx_ = nullptr;
    SSL_CTX* clientCtx_ = nullptr;
    SSL* client_ = nullptr;
    int clientFd_ = -1;
};

class ServeLoop {
public:
    explicit ServeLoop(SocketTls& tls, std::function<void(const std::string&)> onPacket = [](const std::string&) {})
        : thread_([this, &tls, onPacket = std::move(onPacket)] {
              handshakeOk = tls.session->handshake(5000);
              if (handshakeOk)
                  tls.session->serve(onPacket, running);
              finished = true;
          }) {}

    ~ServeLoop() {
        running = false;
        thread_.join();
    }

    std::atomic<bool> running{true};
    std::atomic<bool> handshakeOk{false};
    std::atomic<bool> finished{false};

private:
    std::thread thread_;
};

bool waitFor(const std::atomic<bool>& flag, const int timeoutMs = 3000) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);
    while (!flag.load()) {
        if (std::chrono::steady_clock::now() > deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return true;
}

}

TEST(ClientSession, SendQueuesUntilFlush) {
    TlsPair tls;
    ClientSession session(-1, tls.takeServer());

    ASSERT_TRUE(session.send("one"));
    ASSERT_TRUE(session.send("two"));

    EXPECT_TRUE(session.hasPendingOutput());
    EXPECT_TRUE(tls.clientReadPackets().empty());

    ASSERT_TRUE(session.flush());
    EXPECT_FALSE(session.hasPendingOutput());
    EXPECT_EQ(tls.clientReadPackets(), (std::vector<std::string>{"one", "two"}));
}

TEST(ClientSession, SendFromManyThreadsKeepsPacketsWhole) {
    TlsPair tls;
    ClientSession session(-1, tls.takeServer());
    constexpr int kThreads = 8;
    constexpr int kPackets = 50;

    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&session, t] {
            for (int i = 0; i < kPackets; ++i)
                ASSERT_TRUE(session.send(std::string(100 + t, static_cast<char>('a' + t))));
        });
    }
    for (auto& t : threads)
        t.join();

    ASSERT_TRUE(session.flush());
    const auto packets = tls.clientReadPackets();
    ASSERT_EQ(packets.size(), static_cast<size_t>(kThreads * kPackets));
    for (const auto& p : packets) {
        const int t = p.front() - 'a';
        EXPECT_EQ(p, std::string(100 + t, p.front()));
    }
}

TEST(ClientSession, OverflowingQueueClosesSession) {
    ClientSession session(-1, nullptr);
    const std::string big(MAX_PACKET_SIZE, 'x');

    size_t accepted = 0;
    while (session.send(big))
        ++accepted;

    EXPECT_GT(accepted, 0u);
    EXPECT_LE(accepted * MAX_PACKET_SIZE, MAX_OUTGOING_QUEUE_BYTES);
    EXPECT_TRUE(session.isClosed());
}

TEST(ClientSession, CloseIsReportedOnlyOnceAndRejectsSends) {
    ClientSession session(-1, nullptr);

    EXPECT_TRUE(session.close());
    EXPECT_FALSE(session.close());
    EXPECT_TRUE(session.isClosed());
    EXPECT_FALSE(session.send("late"));
}

TEST(ClientSession, ReceiveRejectsOversizedPacket) {
    TlsPair tls;
    ClientSession session(-1, tls.takeServer());
    const uint32_t len = htonl(MAX_PACKET_SIZE + 1);
    tls.clientWriteRaw(std::string(reinterpret_cast<const char*>(&len), sizeof(len)) + "x");

    std::string message;
    EXPECT_FALSE(session.receive(message));
}

TEST(ClientSessionServe, HandlesPacketsAndReplies) {
    SocketTls tls;
    auto session = tls.session;
    ServeLoop loop(tls, [session](const std::string& msg) {
        ASSERT_TRUE(session->send("echo:" + msg));
    });

    ASSERT_TRUE(tls.clientConnect());
    for (const auto* p : {"one", "two", "three"})
        ASSERT_TRUE(tls.clientSend(p));

    EXPECT_EQ(tls.clientReceive(), "echo:one");
    EXPECT_EQ(tls.clientReceive(), "echo:two");
    EXPECT_EQ(tls.clientReceive(), "echo:three");
    EXPECT_TRUE(loop.handshakeOk);
}

TEST(ClientSessionServe, DeliversPacketsQueuedByOtherThreads) {
    SocketTls tls;
    ServeLoop loop(tls);
    ASSERT_TRUE(tls.clientConnect());
    ASSERT_TRUE(waitFor(loop.handshakeOk));

    std::thread other([&] { ASSERT_TRUE(tls.session->send("from another thread")); });
    other.join();

    EXPECT_EQ(tls.clientReceive(), "from another thread");
}

TEST(ClientSessionServe, DeliversPacketLargerThanSocketBuffer) {
    SocketTls tls;
    ServeLoop loop(tls);
    ASSERT_TRUE(tls.clientConnect());
    ASSERT_TRUE(waitFor(loop.handshakeOk));

    const std::string big(MAX_PACKET_SIZE, 'z');
    ASSERT_TRUE(tls.session->send(big));

    EXPECT_EQ(tls.clientReceive(), big);
}

TEST(ClientSessionServe, CloseFromAnotherThreadStopsLoop) {
    SocketTls tls;
    ServeLoop loop(tls);
    ASSERT_TRUE(tls.clientConnect());
    ASSERT_TRUE(waitFor(loop.handshakeOk));

    EXPECT_TRUE(tls.session->close());

    EXPECT_TRUE(waitFor(loop.finished));
}

TEST(ClientSessionServe, PeerDisconnectStopsLoop) {
    SocketTls tls;
    ServeLoop loop(tls);
    ASSERT_TRUE(tls.clientConnect());
    ASSERT_TRUE(waitFor(loop.handshakeOk));

    tls.clientDisconnect();

    EXPECT_TRUE(waitFor(loop.finished));
}

TEST(ClientSessionServe, InvalidPacketSizeStopsLoop) {
    SocketTls tls;
    std::atomic<int> handled{0};
    ServeLoop loop(tls, [&](const std::string&) { ++handled; });
    ASSERT_TRUE(tls.clientConnect());

    ASSERT_TRUE(tls.clientSend("ok"));
    const uint32_t len = htonl(MAX_PACKET_SIZE + 1);
    ASSERT_TRUE(tls.clientWriteRaw(std::string(reinterpret_cast<const char*>(&len), sizeof(len))));

    EXPECT_TRUE(waitFor(loop.finished));
    EXPECT_EQ(handled.load(), 1);
}

TEST(ClientSession, HandshakeTimesOutWhenClientIsSilent) {
    SocketTls tls;

    const auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(tls.session->handshake(200));
    EXPECT_LT(std::chrono::steady_clock::now() - start, std::chrono::seconds(2));
}
