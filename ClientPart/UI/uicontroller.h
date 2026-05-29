#pragma once

#include <QDialog>
#include <QObject>

#include "mainwindow.h"
#include "loginwindow.h"
#include "MessageRouter.h"
#include "appcontroller.h"
#include "DialogManager.h"


class UIController : public QObject {
    Q_OBJECT
public:
    UIController(MessageRouter* router, AppState* state, Handler* handler, AppController* ctrl, DialogManager* manager);

    void start();
    void showMain();

private:

    MainWindow* mainW;
    LoginWindow* loginW;
    AppController* AController_;
    MessageRouter* router_;
    AppState* state_;
    Handler* handler_;
    DialogManager* manager_;
};
