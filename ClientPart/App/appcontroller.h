#pragma once

#include <QObject>
#include <QTimer>

#include "MessageRouter.h"
#include "AppState.h"
#include "mainwindow.h"
#include "loginwindow.h"
#include "tcp_client.h"

static int64_t PING_TIME_MS = 30000;
static int32_t MAX_RECONNECT_TIME_MS = 30000;

class AppController : public QObject {
    Q_OBJECT

public:
    AppController(MessageRouter* router, AppState* state, Handler* handler, TCPClient* client);
    ~AppController();

    void AttachUI(MainWindow* mainW, LoginWindow* loginW);
    void startPing();
    void stopPing();
    void startReconnect();

signals:
    void ping();

private:
    TCPClient* client_;
    Handler* handler_;
    MessageRouter* router_;
    AppState* state_;
    QTimer* pingTimer = nullptr;
    QTimer* reconnectTimer = nullptr;

    int reconnectAttempts = 0;
};
