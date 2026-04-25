#pragma once

#include <string>

struct Message {
    int senderId;
    int receiverId;
    std::string text;
    int64_t timestamp;
};
