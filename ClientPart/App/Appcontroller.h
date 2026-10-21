#pragma once

#include <QObject>
#include <QTimer>
#include <string>
#include <QtQml>

#include "Router.h"
#include "AppState.h"
#include "Handler.h"
#include "TcpClient.h"
#include "DialogManager.h"

class AppController : public QObject {
    Q_OBJECT

public:
    AppController(Router* router, AppState* state, Handler* handler, TCPClient* client, DialogManager* dialogManager);
    ~AppController();

    Q_INVOKABLE void checkAndResumeSession();
    Q_INVOKABLE bool changeServer(const QString& host, int port);
    Q_INVOKABLE void openDialog(qint64 target_user_id);
    Q_INVOKABLE void sendText(qint64 dialog_id, const QString& text);

public slots:
    void loginRequest(const std::string& login, const std::string& password);
    void registerUser(const std::string& login, const std::string& password);
    void sendMessage(const Message& msg);
    void searchUser(const std::string& text);
    void loadHistory(int64_t dialog_id, int64_t last_msg_id);

    void startPing();
    void stopPing();

signals:
    void resumeSessionFinished(bool success);
    void dialogOpenedSuccessfully(qint64 dialogId, const QString& peerName);
    void dialogOpenFailed(const QString error);

private:
    TCPClient* client_;
    Handler* handler_;
    Router* router_;
    AppState* state_;
    DialogManager* dialogManager_;

    QTimer* pingTimer = nullptr;
    bool pendingResumeSession_ = false;

    static constexpr int64_t PING_TIME_MS = 30000;
};