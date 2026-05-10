#include "MediaHandler.h"

#include <mutex>
#include <filesystem>
#include <iostream>
#include <fstream>

MediaHandler::MediaHandler(MediaContext& m) : mediaContext(m) {}

void MediaHandler::ReadPhoto() {
    try {
        if(std::filesystem::exists(photoFolderPath) && std::filesystem::is_directory(photoFolderPath)) {
            std::filesystem::directory_iterator it{photoFolderPath};
            if(it != std::filesystem::end(it)) {
                std::filesystem::path photoPath = it->path();
                std::string photoName = photoPath.filename().string();

                std::ifstream photo(photoPath, std::ios::binary | std::ios::ate);
                if(photo.is_open()) {
                    std::streamsize photoSize = photo.tellg();
                    photo.seekg(0, std::ios::beg);

                    std::string photoData;
                    photoData.resize(photoSize);
                    photo.read(&photoData[0], photoSize);

                    photo.close();
                    std::filesystem::remove(photoPath);

                    std::lock_guard<std::mutex> lock(mediaContext.photoMutex);
                    mediaContext.photo = "name:" + photoName + "data:" + photoData;
                }
            }
        }
    } catch (const std::filesystem::filesystem_error& e) {
        std::cout << "MediaHandler::ReadPhoto: " << e.what() << std::endl;
    }
}
