#pragma once

#include <string>
#include <vector>

#include "Message.h"

struct Dialog {
    int peerId;
    std::vector<Message> messages;
    std::string lastMessage;
    int64_t timestamp;
};
