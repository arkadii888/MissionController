#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <atomic>
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include <mutex>

#include "mavsdk/plugins/mission/mission.h"

#include <grpcpp/grpcpp.h>
#include "internal_communication.grpc.pb.h"

#include "Vehicle.h"

std::string prompt;
std::mutex promptMutex;

class InternalServiceImplementation final : public InternalService::Service {
public:
    InternalServiceImplementation(Vehicle& v) : vehicle(v) {}

    grpc::Status GetTelemetry(grpc::ServerContext*, const Empty*, TelemetryResponse* reply) {
        auto data = vehicle.GetTelemetry();
        reply->set_latitude_deg(data.latitude_deg);
        reply->set_longitude_deg(data.longitude_deg);
        reply->set_absolute_altitude_m(data.absolute_altitude_m);
        reply->set_relative_altitude_m(data.relative_altitude_m);
        return grpc::Status::OK;
    }

    grpc::Status StartMission(grpc::ServerContext* context, const MissionItemList* request, Empty* reply) override {
        std::vector<mavsdk::Mission::MissionItem> items;
        items.reserve(request->items_size());

        for(auto& receivedItem : request->items()) {
            mavsdk::Mission::MissionItem item{};

            item.latitude_deg = receivedItem.latitude_deg();
            item.longitude_deg = receivedItem.longitude_deg();
            item.relative_altitude_m = receivedItem.relative_altitude_m();
            item.speed_m_s = receivedItem.speed_m_s();
            item.is_fly_through = receivedItem.is_fly_through();
            item.gimbal_pitch_deg = receivedItem.gimbal_pitch_deg();
            item.gimbal_yaw_deg = receivedItem.gimbal_yaw_deg();
            item.camera_action = static_cast<mavsdk::Mission::MissionItem::CameraAction>(receivedItem.camera_action());
            item.loiter_time_s = receivedItem.loiter_time_s();
            item.camera_photo_interval_s = receivedItem.camera_photo_interval_s();
            item.acceptance_radius_m = receivedItem.acceptance_radius_m();
            item.yaw_deg = receivedItem.yaw_deg();
            item.camera_photo_distance_m = receivedItem.camera_photo_distance_m();
            item.vehicle_action = static_cast<mavsdk::Mission::MissionItem::VehicleAction>(receivedItem.vehicle_action());

            items.push_back(item);
        }

        try {
            vehicle.StartMission(items);
        } catch (const std::exception& error) {
            std::cout << "Error: " << error.what() << std::endl;
        }
        return grpc::Status::OK;
    }

    grpc::Status GetPrompt(grpc::ServerContext* context, const Empty* request, PromptResponse* reply) override {
        std::lock_guard<std::mutex> lock(promptMutex);
        reply->set_prompt(prompt);
        prompt = "";
        return grpc::Status::OK;
    }

private:
    Vehicle& vehicle;
};

std::unique_ptr<grpc::Server> internalServer;

void InternalCommunication(Vehicle& vehicle) {
    InternalServiceImplementation service(vehicle);
    grpc::ServerBuilder builder;

    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    internalServer = builder.BuildAndStart();

    internalServer->Wait();
}

std::atomic<bool> communicateWithGroundBase = true;

void GroundBaseCommunication(Vehicle& vehicle) {
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(s, reinterpret_cast<sockaddr*>(&server), sizeof(server));

    char buffer[256];

    while(communicateWithGroundBase) {
        sockaddr_in client;
        socklen_t socklen = sizeof(client);

        memset(buffer, 0, sizeof(buffer));

        int bytesReceived = recvfrom(s, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&client), &socklen);
        if(bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string command(buffer);

            if(command == "#kill") {
                const char* reply = "Killed!";
                try {
                    vehicle.Kill();
                } catch (const std::exception& error) {
                    reply = error.what();
                }
                sendto(s, reply, strlen(reply), 0, reinterpret_cast<sockaddr*>(&client), socklen);
            }
            else if(command == "#telemetry") {
                auto telemetry = vehicle.GetTelemetry();

                std::string reply = "{";
                reply += "\"latitude_deg\":" + std::to_string(telemetry.latitude_deg) + ",";
                reply += "\"longitude_deg\":" + std::to_string(telemetry.longitude_deg) + ",";
                reply += "\"absolute_altitude_m\":" + std::to_string(telemetry.absolute_altitude_m) + ",";
                reply += "\"relative_altitude_m\":" + std::to_string(telemetry.relative_altitude_m) + ",";
                reply += "\"voltage_v\":" + std::to_string(telemetry.voltage_v) + ",";
                reply += "\"current_battery_a\":" + std::to_string(telemetry.current_battery_a) + ",";
                reply += "\"remaining_percent\":" + std::to_string(telemetry.remaining_percent) + ",";
                reply += "}";

                sendto(s, reply.c_str(), reply.length(), 0, reinterpret_cast<sockaddr*>(&client), socklen);
            }
            else {
                std::lock_guard<std::mutex> lock(promptMutex);
                prompt = command;

                const char* reply = "Sent to Agent!";
                sendto(s, reply, strlen(reply), 0, reinterpret_cast<sockaddr*>(&client), socklen);
            }
        }
    }
    close(s);
}

void ClearThreads(std::thread& groundBaseCommunication, std::thread& internalCommunication) {
    communicateWithGroundBase = false;
    if(groundBaseCommunication.joinable()) {
        groundBaseCommunication.join();
    }

    if(internalServer) {
        internalServer->Shutdown();
    }

    if(internalCommunication.joinable()) {
        internalCommunication.join();
    }
}

int main() {
    std::thread groundBaseCommunication;
    std::thread internalCommunication;

    try {
        Vehicle vehicle;

        groundBaseCommunication = std::thread(GroundBaseCommunication, std::ref(vehicle));
        internalCommunication = std::thread(InternalCommunication, std::ref(vehicle));

        while(true) {
            if(vehicle.IsMissionInProgress()) {
                vehicle.TrackMission();
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& error) {
        std::cout << "Error: " << error.what() << std::endl;
    }

    ClearThreads(groundBaseCommunication, internalCommunication);
    return 0;
}
