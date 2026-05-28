#pragma once

#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <iostream>

#include "MetaDialog.h"

class DB_DialogManager {
public:
    explicit DB_DialogManager(const std::string& conn_str);

    void insertDialog(int user_id, int peer_id, int msg_id, const std::string& text, int64_t timestamp);
    std::vector<MetaDialog> getUserDialogs(int user_id);

private:
    pqxx::connection conn;
};
