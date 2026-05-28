#include "DialogManager.h"
#include <QTimer>

DialogManager::DialogManager(Handler* handler, AppState* state) : handler_(handler), state_(state) {}

void DialogManager::start() {
    connect(handler_, &Handler::S_Message, this, [this](const int& sender, const int& receiver, const std::string& text){
        Message msg;
        msg.receiverId = receiver;
        msg.senderId = sender;
        msg.text = text;
        msg.timestamp = now();

        int peer_id = (sender == state_->getCurrentUserId()) ? receiver: sender;
        addMessage(peer_id, msg);
    });

    connect(handler_, &Handler::S_UserSearch, this, [this](const std::vector<User>& users){
        emit findUsers(users);
    });

    connect(handler_, &Handler::S_HistoryLoaded, this, [this](const int& peer_id, std::vector<Message> messages){
        if(messages.empty()) return;
        prependHistory(peer_id, messages);
        emit historyLoaded(peer_id, messages);
    });

    connect(handler_, &Handler::S_DialogsLoaded, this, [this](const std::vector<MetaDialog>& dialogs){
        dialogsLoaded(dialogs);
    });
}

void DialogManager::addMessage(int peer_id, const Message& msg) {
    {
        QMutexLocker locker(&mtx_);

        auto& meta_dialog = data_[peer_id];
        meta_dialog.messages.push_back(msg);
        meta_dialog.meta_dialog_data.last_msg_text = msg.text;
        meta_dialog.meta_dialog_data.last_msg_timestamp = msg.timestamp;
    }

    emit messagesUpdated(peer_id);
    emit dialogsUpdated();
}

const std::vector<Message>* DialogManager::getMessages(int peer_id) const {
    auto it = data_.find(peer_id);
    if(it != data_.end()) return &it->second.messages;
    return nullptr;
}

void DialogManager::prependHistory(int peer_id, const std::vector<Message>& messages) {
    QMutexLocker locker(&mtx_);
    auto& dialog = data_[peer_id];
    dialog.messages.insert(dialog.messages.begin(), messages.begin(), messages.end());
}

void DialogManager::setHistory(int peer_id, const std::vector<Message>& messages) {
    if(messages.empty()) return;

    {
        QMutexLocker locker(&mtx_);
        auto& dialog = data_[peer_id];
        dialog.messages = messages;
        dialog.meta_dialog_data.last_msg_text = messages.back().text;
        dialog.meta_dialog_data.last_msg_timestamp = messages.back().timestamp;
    }

    updateDialog(peer_id);
}

std::vector<MetaDialog> DialogManager::getDialogs() const {
    QMutexLocker locker(&mtx_);
    std::vector<MetaDialog> result;
    result.reserve(data_.size());

    for (const auto& [peer_id, dialog] : data_)
        result.push_back(dialog.meta_dialog_data);

    std::sort(result.begin(), result.end(), [](const MetaDialog& a, const MetaDialog& b){
        return a.last_activity_time > b.last_activity_time;
    });

    return result;
}

void DialogManager::updateDialog(int peer_id) {
    {
        QMutexLocker locker(&mtx_);
        data_[peer_id].meta_dialog_data.last_activity_time = now();
    }

    emit dialogsUpdated();
}

int64_t DialogManager::now() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void DialogManager::dialogsLoaded(const std::vector<MetaDialog>& dialogs) {
    std::unordered_map<int, std::string> users;
    for(auto& d : dialogs) {
        data_[d.peer_id].meta_dialog_data = d;
        users[d.peer_id] = d.username;
    }

    state_->setUsers(users);

    emit dialogsUpdated();
}




































