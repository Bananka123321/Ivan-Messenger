#include "DialogManager.h"
#include "Logging.h"
#include <QTimer>
#include <QDateTime>

DialogManager::DialogManager(Handler* handler, AppState* state) : handler_(handler), state_(state) {
    qCDebug(logDialog) << "Инициализация DialogManager";
}

void DialogManager::start() {
    qCInfo(logDialog) << "Запуск DialogManager. Подключение сигналов";

    connect(handler_, &Handler::S_Message, this, [this](const Message& msg) {
        addMessage(msg.dialog_id, msg);
    });

    connect(handler_, &Handler::S_UserSearch, this, [this](const std::vector<User>& users) {
        qCInfo(logDialog) << "Получены результаты поиска пользователей. Количество:" << users.size();
        emit findUsers(users);
    });

    connect(handler_, &Handler::S_HistoryLoaded, this, [this](int64_t dialog_id, const std::vector<Message>& messages) {
        qCInfo(logDialog) << "Загружена история диалога:" << dialog_id << "сообщений:" << messages.size();
        if (messages.empty()) return;
        prependHistory(dialog_id, messages);
        emit historyLoaded(dialog_id, messages);
    });

    connect(handler_, &Handler::S_DialogsLoaded, this, [this](const std::vector<MetaDialog_Client>& dialogs) {
        qCInfo(logDialog) << "Загружен список диалогов. Количество:" << dialogs.size();
        dialogsLoaded(dialogs);
    });
}

void DialogManager::addMessage(int64_t dialog_id, const Message& msg) {
    bool needUpdateMessages = false;
    bool needUpdateDialogs = false;
    bool isUnknown = false;

    {
        QMutexLocker locker(&mtx_);

        isUnknown = !data_.contains(dialog_id);
        auto& dialog_data = data_[dialog_id];
        dialog_data.meta.dialog_id = dialog_id;

        bool exists = msg.id != 0 && std::any_of(
            dialog_data.messages.begin(),
            dialog_data.messages.end(),
            [&msg](const Message& m) { return m.id == msg.id; }
            );

        if (!exists) {
            dialog_data.messages.push_back(msg);
            needUpdateMessages = true;
            qCDebug(logDialog) << "Новое сообщение в диалоге:" << dialog_id
                               << "msg_id=" << msg.id
                               << "всего сообщений:" << dialog_data.messages.size();
        } else {
            qCDebug(logDialog) << "Дубликат сообщения msg_id=" << msg.id
                               << "в диалоге:" << dialog_id << ". Пропущено";
        }

        dialog_data.meta.last_msg_preview = msg.getPreview();
        dialog_data.meta.last_msg_timestamp = msg.created_at_ms;
        needUpdateDialogs = true;
    }

    if (needUpdateDialogs)
        emit dialogsUpdated();

    if (needUpdateMessages)
        emit messagesUpdated(dialog_id);

    if (isUnknown)
        emit unknownDialog(dialog_id);
}

const std::vector<Message>* DialogManager::getMessages(int64_t dialog_id) const {
    QMutexLocker locker(&mtx_);
    auto it = data_.find(dialog_id);
    if(it != data_.end()) return &it->second.messages;
    qCDebug(logDialog) << "Запрос сообщений для несуществующего диалога:" << dialog_id;
    return nullptr;
}

void DialogManager::prependHistory(int64_t dialog_id, const std::vector<Message>& messages) {
    QMutexLocker locker(&mtx_);
    auto& dialog = data_[dialog_id];

    int addedCount = 0;
    for (const auto& newMsg : messages) {
        bool exists = std::any_of(
            dialog.messages.begin(), dialog.messages.end(),
            [&newMsg](const Message& cachedMsg) {
                return newMsg.id == cachedMsg.id;
            });

        if (!exists) {
            dialog.messages.insert(dialog.messages.begin(), newMsg);
            addedCount++;
        }
    }

    qCDebug(logDialog) << "Добавлено в историю диалога:" << dialog_id
                       << "новых:" << addedCount
                       << "из" << messages.size()
                       << "всего:" << dialog.messages.size();
}

void DialogManager::setHistory(int64_t dialog_id, const std::vector<Message>& messages) {
    if(messages.empty()) return;

    {
        QMutexLocker locker(&mtx_);
        auto& dialog = data_[dialog_id];
        dialog.messages = messages;
        dialog.meta.last_msg_preview = messages.back().getPreview();
        dialog.meta.last_msg_timestamp = messages.back().created_at_ms;
    }

    qCInfo(logDialog) << "История диалога полностью заменена:" << dialog_id
                      << "сообщений:" << messages.size();

    updateDialog(dialog_id);
}

std::vector<MetaDialog_Client> DialogManager::getDialogs() const {
    QMutexLocker locker(&mtx_);
    std::vector<MetaDialog_Client> result;
    result.reserve(data_.size());

    for (const auto& [dialog_id, dialog] : data_) {
        result.push_back(dialog.meta);
    }

    std::sort(result.begin(), result.end(), [](const MetaDialog_Client& a, const MetaDialog_Client& b){
        return a.last_msg_timestamp > b.last_msg_timestamp;
    });

    return result;
}

void DialogManager::updateDialog(int64_t dialog_id) {
    {
        QMutexLocker locker(&mtx_);
        data_[dialog_id].meta.last_msg_timestamp = now();
    }

    qCDebug(logDialog) << "Обновлены метаданные диалога:" << dialog_id;
    emit dialogsUpdated();
}

int64_t DialogManager::now() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void DialogManager::dialogsLoaded(const std::vector<MetaDialog_Client>& dialogs) {
    {
        QMutexLocker locker(&mtx_);
        std::unordered_map<int, std::string> users;
        for(auto& d : dialogs) {
            data_[d.dialog_id].meta = d;
            users[d.peer_id] = d.username;
        }

        qCInfo(logDialog) << "Список диалогов загружен в кэш. Диалогов:" << dialogs.size() << "уникальных пользователей:" << users.size();

        state_->setUsers(users);
    }

    emit dialogsUpdated();
}

void DialogManager::setCurrentDialog(int64_t dialog_id, const User& peer, const std::vector<Message>& messages) {
    {
        QMutexLocker locker(&mtx_);
        auto& dialog = data_[dialog_id];

        dialog.meta.dialog_id = dialog_id;
        dialog.meta.peer_id = peer.user_id;
        dialog.meta.username = peer.username;

        if (!messages.empty()) {
            dialog.messages = messages;
            dialog.meta.last_msg_preview = messages.back().getPreview();
            dialog.meta.last_msg_timestamp = messages.back().created_at_ms;
        } else {
            dialog.messages.clear();
            dialog.meta.last_msg_preview = "Начните общение...";
            dialog.meta.last_msg_timestamp = now();
        }
    }

    qCInfo(logDialog) << "Установлен текущий диалог:" << dialog_id
                      << "с пользователем:" << QString::fromStdString(peer.username)
                      << "загружено сообщений:" << messages.size();

    emit currentDialogChanged(dialog_id);
    emit dialogsUpdated();
}

QVariantList DialogManager::messagesForQml(qint64 dialogId) const {
    QVariantList result;
    const int myId = state_->getCurrentUserId();

    QMutexLocker locker(&mtx_);
    auto it = data_.find(dialogId);
    if (it == data_.end())
        return result;

    const auto& messages = it->second.messages;
    result.reserve(static_cast<qsizetype>(messages.size()));

    for (auto rit = messages.rbegin(); rit != messages.rend(); ++rit) {
        QVariantMap m;
        m["msgId"] = static_cast<qlonglong>(rit->id);
        m["messageText"] = QString::fromStdString(rit->getPreview());
        m["isMine"] = rit->sender_id == myId;
        m["timestamp"] = static_cast<qlonglong>(rit->created_at_ms);
        result.append(m);
    }
    return result;
}

QVariantList DialogManager::dialogsForQml() const {
    QVariantList result;

    for (const auto& d : getDialogs()) {
        QVariantMap m;
        m["chatId"] = static_cast<qlonglong>(d.dialog_id);
        m["peerId"] = d.peer_id;
        m["name"] = QString::fromStdString(d.username);
        m["lastMsg"] = QString::fromStdString(d.last_msg_preview);
        m["time"] = d.last_msg_timestamp > 0 ? QDateTime::fromMSecsSinceEpoch(d.last_msg_timestamp).toString("hh:mm") : QString();
        result.append(m);
    }
    return result;
}