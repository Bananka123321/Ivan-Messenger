#pragma once

#include <QObject>
#include <QSslSocket>
#include <QString>
#include <string>
#include <atomic>
#include <functional>
#include <QTimer>

#include "Router.h"
#include "PacketIO_Client.h"

class TCPClient : public QObject {
    Q_OBJECT

public:
    TCPClient(const QString& host, int port, Router* msgRouter);
    ~TCPClient();

    bool start();
    void disconnect();
    bool isConnected() const;
    void setServer(const QString& host, int port);

    std::function<void(const std::string&)> onMessage;

signals:
    void connectionLose();
    void connected();
    void connecting();

private slots:
    void onEncrypted();
    void onReadyRead();
    void onDisconnected();
    void onSslErrors(const QList<QSslError> &errors);

private:
    QString host_;
    int port_;
    QSslSocket* socket_ = nullptr;
    Router* router_;

    QByteArray m_buffer;
    std::atomic<bool> bConnected{false};

    QTimer* reconnectTimer_ = nullptr;
    int reconnectDelay_ = 1000;

    static constexpr int32_t MAX_RECONNECT_TIME_MS = 30000;

private:
    bool setupSocket();
    void reconnect();
};