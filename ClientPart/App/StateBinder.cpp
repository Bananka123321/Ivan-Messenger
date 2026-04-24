#include "StateBinder.h"

StateChanger::StateChanger(Handler* handler, AppState* state) : handler(handler), state(state) {
    connect(handler, &Handler::S_loginSuccess, this, [state](const std::string& login, const int& user_id){
        state->setCurrentUser(login);
        state->setCurrentUserId(user_id);
    });

    connect(handler, &Handler::S_registerSuccess, this, [state](const std::string& login, const int& user_id){
        state->setCurrentUser(login);
        state->setCurrentUserId(user_id);
    });

    connect(handler, &Handler::S_userList, this, [state](const std::vector<std::string>& users){
        state->setUsers(users);
    });
}

