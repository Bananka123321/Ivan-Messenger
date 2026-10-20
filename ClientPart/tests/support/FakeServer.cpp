#include "FakeServer.h"

#include <QFile>
#include <QSslKey>
#include <QTest>
#include <stdexcept>

#include "PacketIO_Client.h"

namespace {

QByteArray readFile(const char* path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error(std::string("FakeServer: не удалось прочитать ") + path);
    return file.readAll();
}

}

FakeServer::FakeServer() {
    QSslConfiguration config = QSslConfiguration::defaultConfiguration();
    config.setLocalCertificate(QSslCertificate(readFile(TEST_CERT_FILE), QSsl::Pem));
    config.setPrivateKey(QSslKey(readFile(TEST_KEY_FILE), QSsl::Rsa, QSsl::Pem));
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    server_.setSslConfiguration(config);

    connect(&server_, &QSslServer::pendingConnectionAvailable, this, [this] {
        auto* socket = qobject_cast<QSslSocket*>(server_.nextPendingConnection());
        if (!socket)
            return;
        if (client_)
            client_->deleteLater();
        client_ = socket;
        buffer_.clear();
        ++connections_;
        connect(client_, &QSslSocket::readyRead, this, &FakeServer::onReadyRead);
    });

    if (!server_.listen(QHostAddress::LocalHost, 0))
        throw std::runtime_error("FakeServer: listen() не удался");
}

quint16 FakeServer::port() const {
    return server_.serverPort();
}

bool FakeServer::waitForClient(const int timeoutMs) {
    const int before = connections_;
    return QTest::qWaitFor([&] { return connections_ > before && client_ && client_->isEncrypted(); }, timeoutMs);
}

std::optional<nlohmann::json> FakeServer::waitForPacket(const std::string& type, const int timeoutMs) {
    std::optional<nlohmann::json> found;
    QTest::qWaitFor([&] {
        for (auto it = received_.begin(); it != received_.end(); ++it) {
            if (it->value("type", "") == type) {
                found = *it;
                received_.erase(received_.begin(), it + 1);
                return true;
            }
        }
        return false;
    }, timeoutMs);
    return found;
}

std::vector<nlohmann::json> FakeServer::takeReceived() {
    return std::exchange(received_, {});
}

std::vector<std::string> FakeServer::takeTypes(const size_t count, const int timeoutMs) {
    QTest::qWaitFor([&] { return received_.size() >= count; }, timeoutMs);
    QTest::qWait(50);
    std::vector<std::string> types;
    for (const auto& packet : takeReceived())
        types.push_back(packet.value("type", ""));
    return types;
}

void FakeServer::send(const std::string& packet) {
    sendRaw(frame(packet));
}

void FakeServer::sendRaw(const QByteArray& bytes) {
    if (!client_)
        throw std::runtime_error("FakeServer: клиент не подключён");
    client_->write(bytes);
    client_->flush();
}

void FakeServer::dropClient() {
    if (client_) {
        client_->disconnectFromHost();
        client_->deleteLater();
        client_ = nullptr;
    }
}

QByteArray FakeServer::frame(const std::string& payload) {
    const uint32_t len = qToBigEndian(static_cast<uint32_t>(payload.size()));
    QByteArray result(reinterpret_cast<const char*>(&len), sizeof(len));
    result.append(payload.data(), static_cast<qsizetype>(payload.size()));
    return result;
}

void FakeServer::onReadyRead() {
    buffer_.append(client_->readAll());
    while (auto packet = PacketIO_Client::extractPacket(buffer_))
        received_.push_back(nlohmann::json::parse(*packet));
}
