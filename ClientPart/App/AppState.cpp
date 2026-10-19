#include "AppState.h"
#include "Logging.h"

#ifdef Q_OS_ANDROID
const QString IPADRESS_dev = "192.168.0.182";
#else
const QString IPADRESS_dev = "127.0.0.1";
#endif

const QString IPADRESS_dep = "31.192.108.37";
constexpr int DEFAULT_SERVER_PORT = 6767;

AppState::AppState(QObject* parent) : QObject(parent), settings_("IvanMessenger", "ClientConfig") {
    qCDebug(logState) << "Инициализация AppState. Загрузка данных из QSettings";

    sessionToken_ = settings_.value("sessionToken", "").toString();
    user_.user_id = settings_.value("userId", -1).toInt();
    user_.username = settings_.value("username", "").toString().toStdString();
    connectionStatus_ = ConnectionState::Disconnected;
    serverHost_ = settings_.value("serverHost", IPADRESS_dev).toString();
    serverPort_ = settings_.value("serverPort", DEFAULT_SERVER_PORT).toInt();

    qCInfo(logState) << "Загружена сессия: userId=" << user_.user_id
                     << "username=" << QString::fromStdString(user_.username)
                     << "hasToken=" << !sessionToken_.isEmpty()
                     << "server=" << serverHost_ << ":" << serverPort_;
}


void AppState::setUsers(const std::unordered_map<int, std::string>& newUsers) {
    qCDebug(logState) << "Обновление списка пользователей. Количество:" << newUsers.size();
    users_ = newUsers;
    emit usersChanged(users_);
}

const std::unordered_map<int, std::string>& AppState::getUsers() const {
    return users_;
}

QString AppState::getCurrentUsername() const {
    return QString::fromStdString(user_.username);
}

void AppState::setCurrentUsername(const QString& login) {
    qCDebug(logState) << "Изменение username:" << login;
    user_.username = login.toStdString();
    emit usernameChanged();
}

int AppState::getCurrentUserId() const {
    return user_.user_id;
}

void AppState::setCurrentUserId(const int user_id) {
    qCDebug(logState) << "Изменение userId:" << user_id;
    user_.user_id = user_id;
    emit userIdChanged();
}

std::string AppState::getUsernameByUserId(const int id) const {
    auto it = users_.find(id);
    if (it != users_.end())
        return it->second;

    qCDebug(logState) << "Пользователь с id=" << id << "не найден в списке";
    return "Unknown";
}

QString AppState::getSessionToken() const {
    return sessionToken_;
}

void AppState::setSessionToken(const std::string& token) {
    qCDebug(logState) << "Установка sessionToken. Длина:" << token.length();
    sessionToken_ = QString::fromStdString(token);
    emit sessionTokenChanged();
}

std::string AppState::getConnectionToken() const {
    return connectionToken_;
}

void AppState::setConnectionToken(const std::string& token) {
    qCDebug(logState) << "Установка connectionToken. Длина:" << token.length();
    connectionToken_ = token;
}

ConnectionState AppState::getConnectionStatus() const {
    return connectionStatus_;
}

void AppState::setConnectionStatus(ConnectionState newStatus) {
    if(newStatus == connectionStatus_) {
        return;
    }

    qCInfo(logState) << "Изменение статуса соединения:" << static_cast<int>(connectionStatus_)
                     << "->" << static_cast<int>(newStatus);

    connectionStatus_ = newStatus;
    emit connectionStateChanged(newStatus);
}

void AppState::saveSession(const QString& token, int id, const QString& name) {
    qCInfo(logAuth) << "Сохранение сессии: userId=" << id << "username=" << name;

    sessionToken_ = token;
    user_.user_id = id;
    user_.username = name.toStdString();

    settings_.setValue("sessionToken", token);
    settings_.setValue("userId", id);
    settings_.setValue("username", name);

    qCDebug(logAuth) << "Сессия записана в QSettings";

    emit sessionTokenChanged();
    emit userIdChanged();
    emit usernameChanged();
}

void AppState::clearSession() {
    qCInfo(logAuth) << "Очистка сессии. Удаление данных из QSettings";

    sessionToken_ = "";
    user_.user_id = -1;
    user_.username = "";

    settings_.remove("sessionToken");
    settings_.remove("userId");
    settings_.remove("username");

    emit sessionTokenChanged();
    emit userIdChanged();
    emit usernameChanged();
}

bool AppState::hasSession() const {
    bool has = !sessionToken_.isEmpty() && user_.user_id != -1;
    qCDebug(logState) << "Проверка наличия сессии:" << (has ? "да" : "нет");
    return has;
}

QString AppState::getServerHost() const {
    return serverHost_;
}

int AppState::getServerPort() const {
    return serverPort_;
}

void AppState::setServer(const QString& host, int port) {
    qCInfo(logState) << "Изменение адреса сервера:" << serverHost_ << ":" << serverPort_ << "->" << host << ":" << port;

    serverHost_ = host;
    serverPort_ = port;

    settings_.setValue("serverHost", host);
    settings_.setValue("serverPort", port);

    emit serverChanged();
}