#pragma once

#include <QObject>
#include <QTimer>

#include "MessageRouter.h"
#include "AppState.h"
#include "mainwindow.h"
#include "loginwindow.h"

static int64_t PING_TIME_MS = 30000;

class AppController : public QObject {
    Q_OBJECT

public:
    AppController(MessageRouter* router, AppState* state, Handler* handler);
    ~AppController();

    void AttachUI(MainWindow* mainW, LoginWindow* loginW);
    void startPing();
    void stopPing();

signals:
    void ping();

private:
    MessageRouter* router_;
    AppState* state_;
    Handler* handler_;
    QTimer* pingTimer = nullptr;
};
