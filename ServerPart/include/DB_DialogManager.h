#pragma once

#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <iostream>

#include "MetaDialog_Client.h"
#include "Message.h"

class DB_DialogManager {
public:
    explicit DB_DialogManager(const std::string& conn_str);

    void upsertDialog(int user_id, int peer_id, int64_t msg_id, const std::string& preview, int64_t timestamp);
    std::vector<MetaDialog_Client> getUserDialogs(int user_id);
    void updateLastMessage(const Message& msg);
    std::vector<int> getDialogParticipants(int64_t dialog_id);
    std::optional<int64_t> findOrCreatePrivateDialog(int user_a, int user_b);

private:
    pqxx::connection conn_;
};
