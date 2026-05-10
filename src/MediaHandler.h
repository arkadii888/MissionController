#pragma once

#include <string>

#include "MediaContext.h"

class MediaHandler {
public:
    MediaHandler(MediaContext& m);

    void ReadPhoto();
private:
    MediaContext& mediaContext;
    std::string photoFolderPath = "/home/dev/Photo";
};
