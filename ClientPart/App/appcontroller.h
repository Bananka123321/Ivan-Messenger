#pragma once

#include <QObject>
#include "MessageRouter.h"
#include "AppState.h"
#include "mainwindow.h"
#include "loginwindow.h"

class AppController : public QObject {
    Q_OBJECT

public:
    AppController(MessageRouter* router, AppState* state);

    void AttachUI(MainWindow* mainW, LoginWindow* loginW);

private:
    MessageRouter* router;
    AppState* state;
};
