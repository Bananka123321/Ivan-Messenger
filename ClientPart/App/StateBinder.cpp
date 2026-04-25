#include "StateBinder.h"

StateChanger::StateChanger(Handler* handler_, AppState* state_) : handler(handler_), state(state_) {
    connect(handler, &Handler::S_loginSuccess, this, [this](const std::string& login, const int& user_id){
        state->setCurrentUsername(login);
        state->setCurrentUserId(user_id);
    });

    connect(handler, &Handler::S_registerSuccess, this, [this](const std::string& login, const int& user_id){
        state->setCurrentUsername(login);
        state->setCurrentUserId(user_id);
    });

    connect(handler, &Handler::S_userList, this, [this](const std::unordered_map<int, std::string>& users){
        state->setUsers(users);
    });
}

