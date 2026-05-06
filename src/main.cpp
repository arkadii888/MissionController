#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <atomic>
#include <mutex>

#include <mavsdk/log_callback.h>

// UDP
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>

// gRPC
#include <grpcpp/grpcpp.h>
#include "internal_communication.grpc.pb.h"

#include "Vehicle.h"
#include "CommunicationContext.h"
#include "InternalCommunicationImplementation.h"

std::unique_ptr<grpc::Server> internalServer;

void InternalCommunication(Vehicle& vehicle, CommunicationContext& communicationContext) {
    InternalCommunicationImplementation implementation(vehicle, communicationContext);
    grpc::ServerBuilder builder;

    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&implementation);

    internalServer = builder.BuildAndStart();

    internalServer->Wait();
}

std::atomic<bool> communicateExternally = true;

void ExternalCommunication(Vehicle& vehicle, CommunicationContext& communicationContext) {
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

    while(communicateExternally) {
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
                reply += "\"remaining_percent\":" + std::to_string(telemetry.remaining_percent);
                reply += "}";

                sendto(s, reply.c_str(), reply.length(), 0, reinterpret_cast<sockaddr*>(&client), socklen);
            }
            else {
                std::lock_guard<std::mutex> lock(communicationContext.promptMutex);
                communicationContext.prompt = command;

                const char* reply = "Sent to Agent!";
                sendto(s, reply, strlen(reply), 0, reinterpret_cast<sockaddr*>(&client), socklen);
            }
        }
    }
    close(s);
}

void ClearThreads(std::thread& externalCommunication, std::thread& internalCommunication) {
    communicateExternally = false;
    if(externalCommunication.joinable()) {
        externalCommunication.join();
    }

    if(internalServer) {
        internalServer->Shutdown();
    }

    if(internalCommunication.joinable()) {
        internalCommunication.join();
    }
}

int main() {
    mavsdk::log::subscribe([](mavsdk::log::Level level, const std::string& message, const std::string& file, int line) {
        return true;
    });

    std::thread externalCommunication;
    std::thread internalCommunication;

    try {
        Vehicle vehicle;

        CommunicationContext communicationContext;
        externalCommunication = std::thread(ExternalCommunication, std::ref(vehicle), std::ref(communicationContext));
        internalCommunication = std::thread(InternalCommunication, std::ref(vehicle), std::ref(communicationContext));

        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& error) {
        std::cout << "Error: " << error.what() << std::endl; //TODO: React - land or return, depends
    }

    ClearThreads(externalCommunication, internalCommunication);
    return 0;
}
