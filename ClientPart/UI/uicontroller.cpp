#include "uicontroller.h"

UIController::UIController(MessageRouter* router, AppState* state, Handler* handler, AppController* ctrl)
    : router(router), state(state), handler(handler), AController(ctrl) {}

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



    mainW = new MainWindow(nullptr, state, handler);

    connect(handler, &Handler::S_Message, this, [this](const std::string& sender, const std::string& text){
        mainW->newMessage(sender, text);
    });


    AController->AttachUI(mainW, loginW);

    loginW->show();
}

void UIController::showMain() {
    loginW->hide();

    mainW->show();
}
