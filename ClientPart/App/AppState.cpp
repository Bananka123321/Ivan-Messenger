#include "AppState.h"

AppState::AppState() {}

void AppState::setUsers(const std::vector<std::string>& newUsers) {
    users = std::move(newUsers);
    emit usersChanged(users);
}

const std::vector<std::string>& AppState::getUsers() const {
    return users;
}

void AppState::setCurrentUser(const std::string& login) {
    user.currentUser = login;
}

const std::string& AppState::getCurrentUser() const {
    return user.currentUser;
}

void AppState::setCurrentUserId(const int& user_id) {
    user.user_id = user_id;
}

const int& AppState::getCurrentUserId() const {
    return user.user_id;
}
