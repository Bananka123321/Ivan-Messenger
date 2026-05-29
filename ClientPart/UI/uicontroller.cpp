#include "uicontroller.h"

UIController::UIController(MessageRouter* router, AppState* state, Handler* handler, AppController* ctrl, DialogManager* manager)
    : router_(router), state_(state), handler_(handler), AController_(ctrl), manager_(manager) {}

void UIController::start() {
    loginW = new LoginWindow(nullptr, router_);

    connect(handler_, &Handler::S_loginSuccess, loginW, [this](const std::string& login){
        loginW->loginSuccess();
    });

    connect(handler_, &Handler::S_loginFailed, loginW, [this](const std::string& reason){
        loginW->loginFailed(reason);
    });

    connect(handler_, &Handler::S_registerSuccess, loginW, [this](const std::string& login){
        loginW->registerSuccess();
    });

    connect(handler_, &Handler::S_registerFailed, loginW, [this](const std::string& reason){
        loginW->registerFailed(reason);
    });

    connect(loginW, &QDialog::accepted, this, [this](){
        showMain();
    });


    mainW = new MainWindow(nullptr, state_, manager_);

    AController_->AttachUI(mainW, loginW);

    loginW->show();

    manager_->start();
}

void UIController::showMain() {
    loginW->hide();
    mainW->show();

    router_->getDialogsRequest();
}
