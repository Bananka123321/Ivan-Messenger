#pragma once

#include <QObject>
#include <QSslServer>
#include <QSslSocket>
#include <optional>
#include <vector>

#include "nlohmann/json.hpp"

class FakeServer : public QObject {
    Q_OBJECT
public:
    FakeServer();

    [[nodiscard]] quint16 port() const;

    bool waitForClient(int timeoutMs = 5000);

    std::optional<nlohmann::json> waitForPacket(const std::string& type, int timeoutMs = 5000);

    std::vector<nlohmann::json> takeReceived();

    std::vector<std::string> takeTypes(size_t count, int timeoutMs = 5000);

    void send(const std::string& packet);
    void sendRaw(const QByteArray& bytes);

    void dropClient();

    [[nodiscard]] int connectionCount() const { return connections_; }

    static QByteArray frame(const std::string& payload);

    static QSslCertificate certificate();

private:
    void onReadyRead();

    QSslServer server_;
    QSslSocket* client_ = nullptr;
    QByteArray buffer_;
    std::vector<nlohmann::json> received_;
    int connections_ = 0;
};
