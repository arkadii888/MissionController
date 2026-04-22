cmake_minimum_required(VERSION 3.15)
project(MissionController)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions("-Wall -Wextra")

find_package(MAVSDK REQUIRED)
find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)

set(PROTO_SRCS
    src/internal_communication.pb.cc
    src/internal_communication.grpc.pb.cc
)

add_executable(launch
    src/main.cpp
    ${PROTO_SRCS}
)

target_include_directories(launch PRIVATE
    src
    /usr/include/mavsdk
)

target_link_libraries(launch
    MAVSDK::mavsdk
    gRPC::grpc++
    protobuf::libprotobuf
)
