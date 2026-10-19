#define NOMINMAX
#include "Appcontroller.h"
#include "Logging.h"

AppController::AppController(Router* router, AppState* state, Handler* handler, TCPClient* client)
    : router_(router), state_(state), handler_(handler), client_(client) {

    qCDebug(logMain) << "Инициализация AppController";

    connect(handler_, &Handler::S_loginSuccess, this, [this](const std::string& login, const int user_id, const std::string& connectionToken, const std::string& sessionToken){
        qCInfo(logAuth) << "Успешный логин: userId=" << user_id << "username=" << QString::fromStdString(login);
        router_->setReconnecting(false);
        state_->saveSession(QString::fromStdString(sessionToken), user_id, QString::fromStdString(login));
        state_->setConnectionToken(connectionToken);
    });

    connect(handler, &Handler::S_registerSuccess, this, [this](const std::string& login, const int user_id, const std::string& connectionToken, const std::string& sessionToken){
        qCInfo(logAuth) << "Успешная регистрация: userId=" << user_id << "username=" << QString::fromStdString(login);
        router_->setReconnecting(false);
        state_->saveSession(QString::fromStdString(sessionToken), user_id, QString::fromStdString(login));
        state_->setConnectionToken(connectionToken);
    });

    connect(client_, &TCPClient::connectionLose, this, [this](){
        qCWarning(logState) << "Потеря соединения. Остановка ping";
        stopPing();
        state_->setConnectionStatus(ConnectionState::Disconnected);
    });

    connect(client_, &TCPClient::connected, this, [this](){
        qCInfo(logState) << "Соединение установлено";
        state_->setConnectionStatus(ConnectionState::Connected);

        if(pendingResumeSession_) {
            qCInfo(logAuth) << "Выполнение отложенного запроса восстановления сессии";
            pendingResumeSession_ = false;
            router_->resumeSessionRequest(state_->getSessionToken().toStdString());
        } else if(!state_->getConnectionToken().empty()) {
            qCInfo(logAuth) << "Восстановление соединения с токеном";
            router_->resumeConnectionRequest(state_->getConnectionToken());
        }

        startPing();
    });

    connect(client_, &TCPClient::connecting, this, [this](){
        qCDebug(logState) << "Начало подключения к серверу";
        state_->setConnectionStatus(ConnectionState::Connecting);
    });

    connect(handler_, &Handler::S_ResumeConnectionSuccess, this, [this](){
        qCInfo(logAuth) << "Соединение успешно восстановлено";
        router_->setReconnecting(false);
        startPing();
    });

    connect(handler_, &Handler::S_ResumeSessionResponse, this, [this](const bool success, const std::string& token){
        if (success) {
            qCInfo(logAuth) << "Сессия успешно восстановлена";
            state_->setConnectionToken(token);
        } else {
            qCWarning(logAuth) << "Не удалось восстановить сессию";
        }
        emit resumeSessionFinished(success);
    });
}

AppController::~AppController() {
    qCDebug(logMain) << "Уничтожение AppController";
    stopPing();
}

void AppController::loginRequest(const std::string& login, const std::string& password) {
    qCInfo(logAuth) << "Запрос логина для пользователя:" << QString::fromStdString(login);
    router_->loginRequest(login, password);
}

void AppController::registerUser(const std::string& login, const std::string& password) {
    qCInfo(logAuth) << "Запрос регистрации для пользователя:" << QString::fromStdString(login);
    router_->registerRequest(login, password);
}

void AppController::sendMessage(const Message& msg) {
    qCDebug(logDialog) << "Отправка сообщения в диалог:" << msg.dialog_id;
    router_->sendMessage(msg);
}

void AppController::searchUser(const std::string& text) {
    qCInfo(logDialog) << "Поиск пользователя:" << QString::fromStdString(text);
    router_->searchUser(text);
}

void AppController::loadHistory(int64_t dialog_id, int64_t last_msg_id) {
    qCDebug(logDialog) << "Загрузка истории диалога:" << dialog_id << "last_msg_id=" << last_msg_id;
    router_->historyRequest(dialog_id, last_msg_id);
}

void AppController::checkAndResumeSession() {
    qCInfo(logAuth) << "Проверка сессии. Есть сохраненная сессия:" << (state_->hasSession() ? "да" : "нет");

    if (!state_->hasSession()) {
        qCDebug(logAuth) << "Сессия отсутствует. Уведомляем UI";
        emit resumeSessionFinished(false);
        return;
    }

    if(client_->isConnected()) {
        qCInfo(logAuth) << "Соединение активно. Отправка запроса восстановления сессии";
        router_->resumeSessionRequest(state_->getSessionToken().toStdString());
    } else {
        qCInfo(logAuth) << "Соединение отсутствует. Запрос отложен до подключения";
        pendingResumeSession_ = true;
    }
}

void AppController::startPing() {
    if(pingTimer) {
        qCDebug(logRouter) << "Ping таймер уже запущен";
        return;
    }

    qCInfo(logRouter) << "Запуск ping таймера с интервалом:" << PING_TIME_MS << "мс";

    pingTimer = new QTimer(this);

    connect(pingTimer, &QTimer::timeout, this, [this](){
        qCDebug(logRouter) << "Отправка ping";
        router_->ping();
    });
    pingTimer->start(PING_TIME_MS);
}

void AppController::stopPing() {
    if(!pingTimer) {
        qCDebug(logRouter) << "Ping таймер не запущен";
        return;
    }

    qCInfo(logRouter) << "Остановка ping таймера";
    pingTimer->stop();
    pingTimer->deleteLater();
    pingTimer = nullptr;
}

bool AppController::changeServer(const QString& host, int port) {
    const QString trimmed = host.trimmed();
    if (trimmed.isEmpty() || port <= 0 || port > 65535) {
        qCWarning(logState) << "Некорректный адрес сервера:" << host << ":" << port;
        return false;
    }

    qCInfo(logState) << "Переключение на сервер:" << trimmed << ":" << port;

    state_->setServer(trimmed, port);
    state_->setConnectionToken("");
    pendingResumeSession_ = state_->hasSession();

    client_->setServer(trimmed, port);
    return true;
}