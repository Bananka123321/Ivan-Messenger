#pragma once

#include <string>
#include <vector>
#include <QObject>

class AppState : public QObject {
    Q_OBJECT

private:
    struct User {
        std::string currentUser;
        int user_id;
    };

public:
    AppState();

    void setUsers(const std::vector<std::string>& newUsers);
    const std::vector<std::string>& getUsers() const;

    void setCurrentUser(const std::string& login);
    const std::string& getCurrentUser() const;

    void setCurrentUserId(const int& user_id);
    const int& getCurrentUserId() const;

    User user;

signals:
    void usersChanged(const std::vector<std::string>& users);

private:
    std::vector<std::string> users;
};
