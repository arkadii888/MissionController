#pragma once

#include <string>
#include <mutex>

struct CommunicationContext {
    std::string prompt;
    std::mutex promptMutex;
};
