#include "SearchViewModel.h"
#include "Logging.h"

SearchViewModel::SearchViewModel(Router* router, QObject* parent)
    : QObject(parent), router_(router), isSearching_(false) {
    qCDebug(logDialog) << "Инициализация SearchViewModel";
}

QList<QVariantMap> SearchViewModel::getUsers() const {
    return users_;
}

bool SearchViewModel::isSearching() const {
    return isSearching_;
}

void SearchViewModel::searchUser(const QString& username) {
    if (username.trimmed().isEmpty()) {
        qCDebug(logDialog) << "Поле поиска пустое. Очистка результатов";
        users_.clear();
        emit usersChanged();
        return;
    }

    qCInfo(logDialog) << "Поиск пользователя:" << username;
    isSearching_ = true;
    emit isSearchingChanged();

    router_->searchUser(username.toStdString());
}

void SearchViewModel::selectUser(qint64 user_id) {
    qCInfo(logDialog) << "Пользователь выбран для открытия диалога. user_id:" << user_id;
    emit userSelectedForDialog(user_id);
}

void SearchViewModel::onUserSearchResults(const std::vector<User>& users) {
    qCInfo(logDialog) << "Получены результаты поиска. Найдено пользователей:" << users.size();

    users_.clear();
    for (const auto& user : users) {
        QVariantMap userMap;
        userMap["id"] = user.user_id;
        userMap["username"] = QString::fromStdString(user.username);
        users_.append(userMap);
    }

    isSearching_ = false;
    emit usersChanged();
    emit isSearchingChanged();
}