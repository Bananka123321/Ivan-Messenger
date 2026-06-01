#include "AppState.h"

AppState::AppState() {}

void AppState::setUsers(const std::unordered_map<int, std::string>& newUsers) {
    users = newUsers;
    emit usersChanged(users);
}

const std::unordered_map<int, std::string>& AppState::getUsers() const {
    return users;
}

const std::string& AppState::getCurrentUsername() const {
    return user.username;
}

void AppState::setCurrentUsername(const std::string& login) {
    user.username = login;
}

void AppState::setCurrentUserId(const int& user_id) {
    user.user_id = user_id;
}

const int& AppState::getCurrentUserId() const {
    return user.user_id;
}

const std::string AppState::getUsername(const int& id) const {
    auto it = users.find(id);
    if (it != users.end())
        return it->second;

    return "Unknown";
}

const std::string AppState::getCurrentToken() const {
    return user.token;
}

void AppState::setCurrentToken(const std::string& token) {
    user.token = token;
}
