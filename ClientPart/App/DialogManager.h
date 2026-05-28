#pragma once

#include <unordered_map>
#include <QObject>
#include <QMutex>
#include <string>

#include "Message.h"
#include "Handler.h"
#include "AppState.h"
#include "MetaDialog.h"

class DialogManager : public QObject{
    Q_OBJECT

public:
    DialogManager(Handler* handler, AppState* state);

    void start();

    void addMessage(int peer_id, const Message& msg);
    void setHistory(int peer_id, const std::vector<Message>& messages);
    const std::vector<Message>* getMessages(int peer_id) const;
    void prependHistory(int peer_id, const std::vector<Message>& messages);
    void dialogsLoaded(const std::vector<MetaDialog>& dialogs);
    std::vector<MetaDialog> getDialogs() const;

signals:
    void messagesUpdated(int peer_id);
    void historyLoaded(int peerId, const std::vector<Message>& batch);
    void findUsers(const std::vector<User>& users);
    void dialogsUpdated();

private:
    mutable QMutex mtx_;

    struct DialogData {
        std::vector<Message> messages;
        MetaDialog meta_dialog_data;
    };

    std::unordered_map<int, DialogData> data_;

    Handler* handler_;
    AppState* state_;

    void updateDialog(int peer_id);
    int64_t now() const;
};
