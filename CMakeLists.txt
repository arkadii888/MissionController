cmake_minimum_required(VERSION 3.15)

project(MissionController)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-Wall -Wextra")

find_package(MAVSDK REQUIRED)

add_executable(launch src/main.cpp)

target_include_directories(launch PRIVATE /usr/include/mavsdk)

target_link_libraries(launch MAVSDK::mavsdk)
