#include "uicontroller.h"

UIController::UIController(MessageRouter* router_, AppState* state_, Handler* handler_, AppController* ctrl_, DialogManager* manager_)
    : router(router_), state(state_), handler(handler_), AController(ctrl_), manager(manager_) {}

void UIController::start() {
    loginW = new LoginWindow(nullptr, router);

    connect(handler, &Handler::S_loginSuccess, loginW, [this](const std::string& login){
        loginW->loginSuccess();
    });

    connect(handler, &Handler::S_loginFailed, loginW, [this](const std::string& reason){
        loginW->loginFailed(reason);
    });

    connect(handler, &Handler::S_registerSuccess, loginW, [this](const std::string& login){
        loginW->registerSuccess();
    });

    connect(handler, &Handler::S_registerFailed, loginW, [this](const std::string& reason){
        loginW->registerFailed(reason);
    });

    connect(loginW, &QDialog::accepted, this, [this](){
        showMain();
    });


    mainW = new MainWindow(nullptr, state, manager);

    AController->AttachUI(mainW, loginW);

    loginW->show();

    manager->start();
}

void UIController::showMain() {
    loginW->hide();
    mainW->show();

    router->getDialogsRequest();
}
