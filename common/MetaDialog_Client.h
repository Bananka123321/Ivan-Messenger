#pragma once

#include <string>

struct MetaDialog_Client{
    int64_t dialog_id = 0;
    int peer_id = 0;
    std::string username;
    std::string display_name;
    std::string last_msg_preview;
    int64_t last_msg_timestamp = 0;
    int unread_count = 0;
};