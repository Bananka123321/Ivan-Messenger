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
        startPing();
    });

    connect(client_, &TCPClient::connectionLose, this, [this](){
        stopPing();
        startReconnect();
    });

    connect(client_, &TCPClient::connected, this, [this](){
        router_->resumeConnectionRequest(state_->getCurrentToken());
    });

    connect(handler_, &Handler::S_ConnectionSucess, this, [this](){
        startPing();
    });
}

AppController::~AppController() {
    stopPing();
}

void AppController::startPing() {
    if(pingTimer) return;

    std::cerr << "appcontroller: START PING\n";
    pingTimer = new QTimer(this);
    connect(pingTimer, &QTimer::timeout, this, [this](){
        emit ping();
    });
    pingTimer->start(PING_TIME_MS);
}

void AppController::stopPing() {
    if(!pingTimer) return;
    std::cerr << "appcontroller: STOP PING\n";
    pingTimer->stop();
    pingTimer->deleteLater();
    pingTimer = nullptr;
}

void AppController::startReconnect() {
    std::cerr << "appcontroller: START RECONNECT\n";
    if(reconnectTimer && reconnectTimer->isActive()) return;
    if(!reconnectTimer) {
        reconnectTimer = new QTimer(this);
        reconnectTimer->setSingleShot(true);

        connect(reconnectTimer, &QTimer::timeout, this, [this](){
            bool ok = client_->start();
            if(ok) {
                reconnectAttempts = 0;
                startPing();
            } else {
                reconnectAttempts++;
                if(reconnectAttempts <= 10) {
                    int delay = std::min(1000 * (1 << reconnectAttempts), MAX_RECONNECT_TIME_MS);
                    reconnectTimer->start(delay);
                } else {
                    std::cerr << "Failed to reconnect after 10 attempts\n";
                }
            }
        });
    }

    reconnectTimer->start(1000);
}
