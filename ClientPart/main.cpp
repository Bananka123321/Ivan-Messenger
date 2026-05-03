#include "MessageRouter.h"
#include "tcp_client.h"
#include "Handler.h"
#include "uicontroller.h"
#include "appcontroller.h"
#include "StateBinder.h"
#include "DialogManager.h"

#include <QApplication>
#include <QMetaObject>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    AppState* state = new AppState();
    MessageRouter* router = new MessageRouter();
    AppController* AController = new AppController(router, state);
    TCPClient* client = new TCPClient(6767, router);
    Handler* handler = new Handler();
    DialogManager* manager = new DialogManager(handler, state);
    UIController* controller = new UIController(router, state, handler, AController, manager);
    StateChanger* binder = new StateChanger(handler, state);

    client->onMessage = [handler](const std::string& msg) {
        QMetaObject::invokeMethod(handler, [handler, msg]() {
            handler->handleMessage(msg);
        }, Qt::QueuedConnection);
    };

    std::thread([client]() { client->start(); }).detach();

    controller->start();
    a.exec();
    delete client;
    delete handler;
    delete state;
    delete controller;
    return 0;
}
