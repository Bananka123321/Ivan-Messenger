#pragma once

#include <unordered_map>
#include <QObject>
#include <string>

#include "Message.h"
#include "Dialog.h"
#include "Handler.h"
#include "AppState.h"

class DialogManager : public QObject{
    Q_OBJECT

public:
    DialogManager(Handler* handler_, AppState* state_);

    void start();
    void addMessage(const Message& msg);

    const std::unordered_map<int, Dialog>& getDialogs() const;
    const Dialog* getDialog(int peerId) const;

signals:
    void dialogsUpdated();
    void dialogUpdated(int peerId);

    void findUsers(const std::vector<protocol::User>& users);

private:
    std::unordered_map<int, Dialog> dialogs;

    Handler* handler;
    AppState* state;
};
