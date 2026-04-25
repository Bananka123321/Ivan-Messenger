#include "DialogManager.h"
#include <QTimer>

DialogManager::DialogManager(Handler* handler_) : handler(handler_) {}

void DialogManager::start() {
    connect(handler, &Handler::S_Message, this, [this](const int& sender, const std::string& text){
        Message msg;
        msg.senderId = sender;
        msg.text = text;

        addMessage(msg);
    });

    emit dialogsUpdated();

    Message m1{1, 0, "Hello from user 1"};
    Message m2{2, 0, "Hello from user 2"};
    Message m3{1, 0, "Second message"};

    QTimer::singleShot(10000, this, [this]() {
        addMessage({1, 0, "Hello from user 1"});
    });

    QTimer::singleShot(15000, this, [this]() {
        addMessage({2, 0, "Hello from user 2"});
    });

    QTimer::singleShot(30000, this, [this]() {
        addMessage({1, 0, "Second message"});
    });
}

void DialogManager::addMessage(Message msg) {
    int peerId = msg.senderId;

    Dialog& dialog = dialogs[peerId];
    dialog.peerId = peerId;

    dialog.messages.push_back(msg);
    dialog.lastMessage = msg.text;

    emit dialogUpdated(peerId);
}

const std::unordered_map<int, Dialog>& DialogManager::getDialogs() const {
    return dialogs;
}

const Dialog* DialogManager::getDialog(int peerId) const {
    auto it = dialogs.find(peerId);
    if(it != dialogs.end()) return &it->second;

    return nullptr;
}
