#pragma once

#include <string>
#include <vector>
#include "Message.h"

struct MetaDialog{
    int peer_id;
    std::string username;
    std::string last_msg_text;
    int64_t last_msg_timestamp;
    int64_t last_activity_time;
};