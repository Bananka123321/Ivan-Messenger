#include "StateBinder.h"

StateChanger::StateChanger(Handler* handler, AppState* state) : handler_(handler), state_(state) {
    connect(handler, &Handler::S_loginSuccess, this, [this](const std::string& login, const int& user_id, const std::string& token){
        state_->setCurrentUsername(login);
        state_->setCurrentUserId(user_id);
        state_->setCurrentToken(token);
    });

    connect(handler, &Handler::S_registerSuccess, this, [this](const std::string& login, const int& user_id, const std::string& token){
        state_->setCurrentUsername(login);
        state_->setCurrentUserId(user_id);
        state_->setCurrentToken(token);
    });

    connect(handler, &Handler::S_userList, this, [this](const std::unordered_map<int, std::string>& users){
        state_->setUsers(users);
    });
}

