#include "appcontroller.h"

AppController::AppController(MessageRouter* router, AppState* state, Handler* handler) : router_(router), state_(state), handler_(handler) {}

void AppController::AttachUI(MainWindow* mainW, LoginWindow* loginW) {
    connect(mainW, &MainWindow::sendMessageRequest, this, [this](const int& to, const std::string& text) {
        router_->sendMessage(state_->getCurrentUserId(), to, text);
    });

    connect(loginW, &LoginWindow::loginRequest, this, [this](const std::string& login, const std::string& password) {
        router_->loginRequest(login, password);
    });

    connect(loginW, &LoginWindow::registerRequest, this, [this](const std::string& login, const std::string& password) {
        router_->registerRequest(login, password);
    });

    connect(mainW, &MainWindow::searchUser, this, [this](const std::string& text){
        router_->searchUser(text);
    });

    connect(mainW, &MainWindow::loadHistoryRequest, this, [this](int peer_id, int last_msg_id){
        router_->historyRequest(peer_id, last_msg_id);
    });

    connect(this, &AppController::ping, router_, &MessageRouter::ping, Qt::QueuedConnection);

    connect(handler_, &Handler::S_loginSuccess, this, [this](const std::string&, int){
        startPing();
    });
}

AppController::~AppController() {
    stopPing();
}

void AppController::startPing() {
    if(pingTimer) return;

    pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, [this](){
        emit ping();
    });
    pingTimer->start(PING_TIME_MS);
}

void AppController::stopPing() {
    if(!pingTimer) return;
    pingTimer->stop();
    delete pingTimer;
    pingTimer = nullptr;
}

