#include "appcontroller.h"

AppController::AppController(MessageRouter* router, AppState* state) : router(router), state(state) {}

void AppController::AttachUI(MainWindow* mainW, LoginWindow* loginW) {
    connect(mainW, &MainWindow::sendMessageRequest, this, [this](const int& to, const std::string& text) {
        router->sendMessage(state->getCurrentUserId(), to, text);
    });

    connect(loginW, &LoginWindow::loginRequest, this, [this](const std::string& login, const std::string& password) {
        router->loginRequest(login, password);
    });

    connect(loginW, &LoginWindow::registerRequest, this, [this](const std::string& login, const std::string& password) {
        router->registerRequest(login, password);
    });
}


