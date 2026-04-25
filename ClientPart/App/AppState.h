#pragma once

#include <string>
#include <QObject>
#include <unordered_map>

class AppState : public QObject {
    Q_OBJECT

private:
    struct User {
        std::string username;
        int user_id;
    };

public:
    AppState();

    const std::unordered_map<int, std::string>& getUsers() const;
    void setUsers(const std::unordered_map<int, std::string>& newUsers);

    const std::string& getCurrentUsername() const;
    void setCurrentUsername(const std::string& login);

    const int& getCurrentUserId() const;
    void setCurrentUserId(const int& user_id);

    const std::string getUsername(const int& id) const;

    User user;

signals:
    void usersChanged(const std::unordered_map<int, std::string>& users);

private:
    std::unordered_map<int, std::string> users;
};
