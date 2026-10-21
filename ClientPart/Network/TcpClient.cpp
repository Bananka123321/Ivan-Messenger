#include "TcpClient.h"
#include "Logging.h"

#include <QSslConfiguration>
#include <QSslKey>
#include <algorithm>

TCPClient::TCPClient(const QString& host, int port, Router* msgRouter)
    : host_(host), port_(port), router_(msgRouter), socket_(nullptr), bConnected(false), reconnectDelay_(1000),
      trustedCertificates_(QSslCertificate::fromPath(QStringLiteral(":/certs/server.crt"))) {}

TCPClient::~TCPClient() {
    qCDebug(logTcp) << "Уничтожение TCPClient";
    disconnect();
    if(reconnectTimer_) {
        reconnectTimer_->deleteLater();
    }
}

bool TCPClient::start() {
    if (socket_ && socket_->state() == QAbstractSocket::ConnectedState) {
        qCDebug(logTcp) << "start() вызван, но сокет уже подключен. Пропускаем.";
        return true;
    }

    qCInfo(logTcp) << "Инициализация подключения...";
    return setupSocket();
}

bool TCPClient::setupSocket() {
    qCInfo(logTcp) << "Настройка сокета. Цель:" << host_ << ":" << port_;

    if(socket_) {
        qCDebug(logTcp) << "Старый сокет существует, очищаем его";
        socket_->disconnectFromHost();
        socket_->deleteLater();
        socket_ = nullptr;
    }

    socket_ = new QSslSocket(this);

    connect(socket_, &QSslSocket::encrypted, this, &TCPClient::onEncrypted);
    connect(socket_, &QSslSocket::readyRead, this, &TCPClient::onReadyRead);
    connect(socket_, &QSslSocket::disconnected, this, &TCPClient::onDisconnected);
    connect(socket_, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this, &TCPClient::onSslErrors);

    connect(socket_, &QAbstractSocket::errorOccurred, this, [this](QAbstractSocket::SocketError error) {
        qCWarning(logTcp) << "Ошибка сокета:" << error << "|" << socket_->errorString();
        bConnected.store(false);
        reconnect();
    });

    QSslConfiguration sslConfig = socket_->sslConfiguration();
    sslConfig.setCaCertificates(trustedCertificates_);
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyPeer);
    sslConfig.setProtocol(QSsl::TlsV1_2OrLater);
    socket_->setSslConfiguration(sslConfig);

    socket_->connectToHostEncrypted(host_, port_);
    emit connecting();

    return true;
}

void TCPClient::onEncrypted() {
    qCInfo(logTcp) << "TLS-соединение успешно установлено!";
    bConnected.store(true);
    router_->setSocket(socket_);

    reconnectDelay_ = 1000;

    emit connected();
}

void TCPClient::onReadyRead() {
    QByteArray newData = socket_->readAll();

    qCDebug(logTcp) << "Получено новых данных:" << newData.size() << "байт";

    m_buffer.append(newData);

    while (auto packet = PacketIO_Client::extractPacket(m_buffer)) {
        qCDebug(logTcp) << "Успешно извлечён пакет из буфера";
        if (onMessage) {
            onMessage(*packet);
        }
    }
}

void TCPClient::onDisconnected() {
    qCWarning(logTcp) << "Соединение разорвано. Причина: " << (socket_ ? socket_->errorString() : " сокет уже уничтожен");

    bConnected.store(false);
    router_->setSocket(nullptr);

    if(socket_) {
        socket_->deleteLater();
        socket_ = nullptr;
    }

    emit connectionLose();
}

void TCPClient::onSslErrors(const QList<QSslError> &errors) {
    if (isTrusted(socket_->peerCertificate())) {
        qCDebug(logTcp) << "Ключ сервера совпадает с доверенным, ошибки проверки сертификата пропущены:" << errors.size();
        socket_->ignoreSslErrors(errors);
        return;
    }

    qCWarning(logTcp) << "Сертификат сервера не совпадает с доверенным, соединение отклонено:";
    for (const auto &error : errors) {
        qCWarning(logTcp) << "   -" << error.errorString();
    }
}

bool TCPClient::isTrusted(const QSslCertificate& certificate) const {
    if (certificate.isNull()) {
        return false;
    }

    const QByteArray key = certificate.publicKey().toDer();
    return std::any_of(trustedCertificates_.cbegin(), trustedCertificates_.cend(), [&key](const QSslCertificate& trusted) {
        return trusted.publicKey().toDer() == key;
    });
}

void TCPClient::setTrustedCertificates(const QList<QSslCertificate>& certificates) {
    trustedCertificates_ = certificates;
}

void TCPClient::disconnect() {
    if (!bConnected.load() || !socket_) {
        qCDebug(logTcp) << "disconnect() вызван, но сокет уже неактивен. Ничего не делаем.";
        return;
    }

    qCInfo(logTcp) << "Принудительное отключение сокета";
    router_->setSocket(nullptr);

    socket_->disconnectFromHost();

    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->waitForDisconnected(1000);
    }

    if(socket_) {
        socket_->deleteLater();
        socket_ = nullptr;
    }

    bConnected.store(false);
}

bool TCPClient::isConnected() const {
    return bConnected.load() && socket_ && socket_->state() == QAbstractSocket::ConnectedState;
}

void TCPClient::reconnect() {
    if(reconnectTimer_ && reconnectTimer_->isActive()) {
        qCDebug(logTcp) << "Таймер переподключения уже активен, пропускаем вызов reconnect()";
        return;
    }

    if(!reconnectTimer_) {
        reconnectTimer_ = new QTimer(this);
        reconnectTimer_->setSingleShot(true);
        connect(reconnectTimer_, &QTimer::timeout, this, &TCPClient::start);
    }

    qCInfo(logTcp) << "Переподключение запланировано через" << reconnectDelay_ << "мс";

    reconnectTimer_->start(reconnectDelay_);

    reconnectDelay_ = std::min(reconnectDelay_ * 2, MAX_RECONNECT_TIME_MS);
}

void TCPClient::setServer(const QString& host, int port) {
    qCInfo(logTcp) << "Смена сервера на" << host << ":" << port;

    host_ = host;
    port_ = port;

    if(reconnectTimer_) {
        reconnectTimer_->stop();
    }
    reconnectDelay_ = 1000;

    if(socket_) {
        QSslSocket* oldSocket = socket_;
        socket_ = nullptr;
        router_->setSocket(nullptr);

        oldSocket->disconnect(this);
        oldSocket->abort();
        oldSocket->deleteLater();
    }

    if(bConnected.exchange(false)) {
        emit connectionLose();
    }

    start();
}