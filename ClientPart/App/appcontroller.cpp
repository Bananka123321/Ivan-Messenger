#define NOMINMAX
#include "appcontroller.h"

AppController::AppController(MessageRouter* router, AppState* state, Handler* handler, TCPClient* client) : router_(router), state_(state),  handler_(handler), client_(client) {}

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

    connect(handler_, &Handler::S_loginSuccess, this, [this](const std::string&, int, const std::string&){
        router_->setReconnecting(false);
        startPing();
    });

    connect(client_, &TCPClient::connectionLose, this, [this](){
        stopPing();
        startReconnect();
    });

    connect(client_, &TCPClient::connected, this, [this](){
        if(state_->getCurrentToken().empty())
            return;
        router_->resumeConnectionRequest(state_->getCurrentToken());
    });

    connect(handler_, &Handler::S_ConnectionSucess, this, [this](){
        router_->setReconnecting(false);
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
    pingTimer->deleteLater();
    pingTimer = nullptr;
}

void AppController::startReconnect() {
    router_->setReconnecting(true);

    if(reconnectTimer && reconnectTimer->isActive()) return;
    if(!reconnectTimer) {
        reconnectTimer = new QTimer(this);
        reconnectTimer->setSingleShot(true);

        connect(reconnectTimer, &QTimer::timeout, this, [this](){
            bool ok = client_->start();
            if(ok) {
                reconnectAttempts = 0;
            } else {
                reconnectAttempts++;
                if(reconnectAttempts <= 10) {
                    int delay = std::min(1000 * (1 << reconnectAttempts), MAX_RECONNECT_TIME_MS);
                    reconnectTimer->start(delay);
                } else {
                    router_->setReconnecting(false);
                }
            }
        });
    }

    reconnectTimer->start(1000);
}
