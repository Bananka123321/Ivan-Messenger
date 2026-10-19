#include "Router.h"
#include "Logging.h"
#include <QtEndian>

Router::Router() : socket_(nullptr) {
    qCDebug(logRouter) << "Инициализация Router";
}

void Router::setSocket(QSslSocket* socket) {
    std::lock_guard<std::mutex> lock(mutex_);
    socket_ = socket;
    qCDebug(logRouter) << "Установка сокета:" << (socket_ ? "успешно" : "nullptr (очистка)");
}

void Router::setReconnecting(bool value) {
    bool oldVal = isReconnecting_.load();
    isReconnecting_.store(value);
    if (oldVal != value) {
        qCDebug(logRouter) << "Статус переподключения изменен на:" << (value ? "true" : "false");
    }
}

void Router::loginRequest(const std::string& login, const std::string& password) {
    qCInfo(logRouter) << "Формирование запроса на вход для пользователя:" << QString::fromStdString(login);
    std::string request = protocol::loginRequest(login, password);
    sendPacket(request);
}

void Router::registerRequest(const std::string& login, const std::string& password) {
    qCInfo(logRouter) << "Формирование запроса на регистрацию для пользователя:" << QString::fromStdString(login);
    std::string request = protocol::registerRequest(login, password);
    sendPacket(request);
}

void Router::sendMessage(const Message &msg) {
    qCDebug(logRouter) << "Формирование пакета сообщения для диалога:" << msg.dialog_id;
    std::string request = protocol::sendMessage(msg);
    sendPacket(request);
}

void Router::searchUser(const std::string& text) {
    qCInfo(logRouter) << "Формирование запроса поиска пользователя:" << QString::fromStdString(text);
    std::string request = protocol::searchUserRequest(text);
    sendPacket(request);
}

void Router::historyRequest(const int64_t dialog_id, const int64_t last_msg_id) {
    qCDebug(logRouter) << "Формирование запроса истории. dialog_id:" << dialog_id << "last_msg_id:" << last_msg_id;
    std::string request = protocol::historyRequest(dialog_id, last_msg_id, 200);
    sendPacket(request);
}

void Router::getDialogsRequest() {
    qCInfo(logRouter) << "Формирование запроса списка диалогов";
    std::string request = protocol::getDialogsRequest();
    sendPacket(request);
}

void Router::ping() {
    qCDebug(logRouter) << "Формирование ping-запроса";
    std::string request = protocol::ping();
    sendPacket(request);
}

void Router::resumeConnectionRequest(const std::string& token) {
    qCInfo(logRouter) << "Формирование запроса на восстановление соединения";
    std::string request = protocol::resumeConnectionRequest(token);
    sendPacket(request, true);
}

void Router::resumeSessionRequest(const std::string& token) {
    qCInfo(logRouter) << "Формирование запроса на восстановление сессии";
    std::string request = protocol::resumeSessionRequest(token);
    sendPacket(request, true);
}

void Router::sendPacket(const std::string& msg, bool force) {
    if (!force && isReconnecting_.load()) {
        qCDebug(logRouter) << "Отправка пакета отменена: активен флаг переподключения";
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (!force && isReconnecting_.load()) {
        return;
    }

    if (!socket_) {
        qCWarning(logRouter) << "Ошибка отправки: сокет равен nullptr";
        return;
    }

    if (socket_->state() != QAbstractSocket::ConnectedState) {
        qCWarning(logRouter) << "Ошибка отправки: сокет не в состоянии Connected (текущее состояние:" << socket_->state() << ")";
        return;
    }

    QSslSocket* sslSocket = qobject_cast<QSslSocket*>(socket_);
    if (sslSocket && !sslSocket->isEncrypted()) {
        qCWarning(logRouter) << "Ошибка отправки: SSL-соединение не зашифровано";
        return;
    }

    try {
        uint32_t len = qToBigEndian(static_cast<uint32_t>(msg.size()));
        qint64 written1 = socket_->write(reinterpret_cast<const char*>(&len), sizeof(len));
        qint64 written2 = socket_->write(msg.data(), msg.size());

        if (written1 == -1 || written2 == -1) {
            qCWarning(logRouter) << "Сбой записи в сокет:" << socket_->errorString();
        } else {
            qCDebug(logRouter) << "Успешная отправка пакета. Размер данных:" << msg.size() << "байт";
        }
    } catch (const std::exception& e) {
        qCCritical(logRouter) << "Критическое исключение при записи в сокет:" << e.what();
    }
}

void Router::openDialogRequest(int64_t target_user_id) {
    qCInfo(logRouter) << "Формирование запроса на открытие диалога с user_id:" << target_user_id;
    std::string request = protocol::openDialogRequest(target_user_id);
    sendPacket(request);
}