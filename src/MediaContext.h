#pragma once

#include <string>
#include <mutex>

struct MediaContext {
    std::string photo;
    std::mutex photoMutex;
};
