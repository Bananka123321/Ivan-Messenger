#pragma once

#include <QDialog>
#include <QObject>

#include "mainwindow.h"
#include "loginwindow.h"
#include "MessageRouter.h"
#include "appcontroller.h"


class UIController : public QObject {
    Q_OBJECT
public:
    UIController(MessageRouter* router, AppState* state, Handler* handler, AppController* ctrl);

    void start();
    void showMain();

private:

    MainWindow* mainW;
    LoginWindow* loginW;
    AppController* AController;
    MessageRouter* router;
    AppState* state;
    Handler* handler;
};
