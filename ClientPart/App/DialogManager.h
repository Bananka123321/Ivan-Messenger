#pragma once

#include <unordered_map>
#include <QObject>
#include <QMutex>
#include <string>

#include "Message.h"
#include "Handler.h"
#include "AppState.h"
#include "MetaDialog_Client.h"

class DialogManager : public QObject{
    Q_OBJECT
    Q_PROPERTY(QVariantList dialogs READ dialogsForQml NOTIFY dialogsUpdated)

public:
    DialogManager(Handler* handler, AppState* state);

    void start();

    void addMessage(int64_t dialog_id, const Message& msg);
    void setHistory(int64_t dialog_id, const std::vector<Message>& messages);
    const std::vector<Message>* getMessages(int64_t dialog_id) const;
    void prependHistory(int64_t dialog_id, const std::vector<Message>& messages);
    void dialogsLoaded(const std::vector<MetaDialog_Client>& dialogs);
    std::vector<MetaDialog_Client> getDialogs() const;

    void setCurrentDialog(int64_t dialog_id, const User& peer, const std::vector<Message>& messages);

    Q_INVOKABLE QVariantList messagesForQml(qint64 dialogId) const;
    QVariantList dialogsForQml() const;

signals:
    void messagesUpdated(qint64 dialogId);
    void historyLoaded(int64_t dialog_id, const std::vector<Message>& batch);
    void findUsers(const std::vector<User>& users);
    void dialogsUpdated();
    void currentDialogChanged(int64_t dialog_id);
    void unknownDialog(int64_t dialog_id);

private:
    mutable QMutex mtx_;

    struct DialogData {
        std::vector<Message> messages;
        MetaDialog_Client meta;
    };

    std::unordered_map<int64_t, DialogData> data_;

    Handler* handler_;
    AppState* state_;

    void updateDialog(int64_t dialog_id);
    int64_t now() const;
};
