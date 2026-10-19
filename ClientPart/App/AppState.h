#pragma once

#include <string>
#include <QObject>
#include <QSettings>
#include <unordered_map>
#include <QtQml>

#include "User.h"

enum class ConnectionState {
    Disconnected = 0,
    Connecting = 1,
    Connected = 2
};


Q_ENUMS(ConnectionState)

class AppState : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString sessionToken READ getSessionToken NOTIFY sessionTokenChanged FINAL)
    Q_PROPERTY(QString username READ getCurrentUsername NOTIFY usernameChanged FINAL)
    Q_PROPERTY(int userId READ getCurrentUserId NOTIFY userIdChanged FINAL)
    Q_PROPERTY(ConnectionState connectionStatus READ getConnectionStatus NOTIFY connectionStateChanged FINAL)
    Q_PROPERTY(QString serverHost READ getServerHost NOTIFY serverChanged FINAL)
    Q_PROPERTY(int serverPort READ getServerPort NOTIFY serverChanged FINAL)

public:
    explicit AppState(QObject* parent = nullptr);

    const std::unordered_map<int, std::string>& getUsers() const;
    void setUsers(const std::unordered_map<int, std::string>& newUsers);

    QString getCurrentUsername() const;
    void setCurrentUsername(const QString& login);

    int getCurrentUserId() const;
    void setCurrentUserId(const int user_id);

    QString getSessionToken() const;
    void setSessionToken(const std::string& token);

    std::string getConnectionToken() const;
    void setConnectionToken(const std::string& token);

    std::string getUsernameByUserId(const int id) const;

    ConnectionState getConnectionStatus() const;
    void setConnectionStatus(ConnectionState newStatus);

    QString getServerHost() const;
    int getServerPort() const;
    void setServer(const QString& host, int port);

    Q_INVOKABLE void saveSession(const QString& token, int id, const QString& username);
    Q_INVOKABLE void clearSession();
    Q_INVOKABLE bool hasSession() const;

signals:
    void usersChanged(const std::unordered_map<int, std::string>& users);
    void sessionTokenChanged();
    void usernameChanged();
    void userIdChanged();
    void connectionStateChanged(ConnectionState status);
    void serverChanged();

private:
    User user_;
    std::unordered_map<int, std::string> users_;
    std::string connectionToken_;
    QString sessionToken_;
    QSettings settings_;
    ConnectionState connectionStatus_;
    QString serverHost_;
    int serverPort_;
};
