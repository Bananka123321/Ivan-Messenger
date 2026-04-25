#pragma once

#include <unordered_map>
#include <QObject>
#include <string>

#include "Message.h"
#include "Dialog.h"
#include "Handler.h"

class DialogManager : public QObject{
    Q_OBJECT

public:
    DialogManager(Handler* handler_);

    void start();
    void addMessage(Message msg);

    const std::unordered_map<int, Dialog>& getDialogs() const;
    const Dialog* getDialog(int peerId) const;

signals:
    void dialogsUpdated();
    void dialogUpdated(int peerId);

private:
    std::unordered_map<int, Dialog> dialogs;

    Handler* handler;
};
