#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <atomic>
#include <unistd.h>
#include <sys/time.h>
#include <vector>
#include <cmath>

#include "mavsdk/mavsdk.h"
#include "mavsdk/plugins/action/action.h"
#include "mavsdk/plugins/telemetry/telemetry.h"
#include <mavsdk/plugins/mission/mission.h>

#include <grpcpp/grpcpp.h>
#include "internal_communication.grpc.pb.h"

#include "Vehicle.h"

using namespace mavsdk;

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

        vehicle.Arm();
        vehicle.CompleteMission();
        vehicle.ClearMission();
    } catch (const std::exception& error) {
        std::cout << "Error: " << error.what() << std::endl;
        ClearThreads(groundBaseCommunication, internalCommunication);
    }

    ClearThreads(groundBaseCommunication, internalCommunication);
    return 0;
}
