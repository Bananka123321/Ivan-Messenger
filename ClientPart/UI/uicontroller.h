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
    UIController(MessageRouter* router_, AppState* state_, Handler* handler_, AppController* ctrl_, DialogManager* manager_);

    void start();
    void showMain();

private:

    MainWindow* mainW;
    LoginWindow* loginW;
    AppController* AController;
    MessageRouter* router;
    AppState* state;
    Handler* handler;
    DialogManager* manager;
};
