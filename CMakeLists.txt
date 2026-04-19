cmake_minimum_required(VERSION 3.15)

project(MissionController)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-Wall -Wextra -Werror")

find_package(MAVSDK REQUIRED)

add_executable(takeoff_test src/main.cpp)

add_executable(takeoff_test src/main.cpp)
