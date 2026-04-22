#include <chrono>
#include <iostream>
#include <memory>
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

using namespace mavsdk;

class InternalServiceImplementation {
public:
    InternalServiceImplementation(Telemetry& t) : telemetry(t) {}

    grpc::Status GetTelemetry(grpc::ServerContext* context, const Empty* request, TelemetryResponse* reply) {
        reply->set_current_latitude(telemetry.position().latitude_deg);
        reply->set_current_logitude(telemetry.position().longitude_deg);
        return grpc::Status::OK;
    }

private:
    Telemetry& telemetry;
};

std::unique_ptr<grpc::Server> internalServer;

void InternalCommunication(Telemetry& telemetry) {
    InternalServiceImplementation service();
    grpc::ServerBuilder builder;

    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    internalServer = builder.BuildAndStart();

    internalServer->Wait();
}

std::atomic<bool> communicateWithGroundBase = true;

void GroundBaseCommunication(Action& action) {
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
                auto killResult = action.kill();
                const char* reply;
                if(killResult != Action::Result::Success) {
                    reply = "Kill Failed.";
                }
                else {
                    reply = "Killed!";
                }
                sendto(s, reply, strlen(reply), 0, reinterpret_cast<sockaddr*>(&client), socklen);
            }
        }
    }
    close(s);
}

int main() {
    Mavsdk mavsdk(Mavsdk::Configuration(ComponentType::GroundStation));

    std::cout << "MissionController: Connecting To Vehicle..." << std::endl;

    ConnectionResult connectionResult = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");
    if (connectionResult != ConnectionResult::Success) {
        std::cout << "MissionController: Connection Failed: " << static_cast<int>(connectionResult) << "." << std::endl;
        return 1;
    }

    std::cout << "MissionController: Vehicle Connected!" << std::endl;
    std::cout << "MissionController: Detecting Vehicle..." << std::endl;

    while(mavsdk.systems().empty()) {
        std::cout << "MissionController: Vehicle Not Detected Yet..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Vehicle Detected!" << std::endl;

    auto system = mavsdk.systems()[0];
    auto action = Action(system);
    auto telemetry = Telemetry(system);
    auto mission = Mission(system);

    std::thread groundBaseCommunication(GroundBaseCommunication, std::ref(action));
    std::thread internalCommunication(InternalCommunication, std::ref(telemetry));

    std::cout << "MissionController: Checking Health..." << std::endl;

    while(!telemetry.health_all_ok()) {
        std::cout << "MissionController: Vehicle Not Ready To Arm..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Ready To Arm!" << std::endl;
    std::cout << "MissionController: Defining Mission..." << std::endl;

    std::vector<Mission::MissionItem> missionItems;

    double firstLatitude = telemetry.position().latitude_deg;
    double currentLongitude = telemetry.position().longitude_deg;

    double secondLatitude = firstLatitude + 0.00009f;
    double thirdLatitude = secondLatitude + 0.00009f;
    double fourthLatitude = thirdLatitude + 0.00009f;

    Mission::MissionItem item{};
    item.latitude_deg = firstLatitude;
    item.longitude_deg = currentLongitude;
    item.relative_altitude_m = 5.0f;
    item.acceptance_radius_m = 0.5f;
    item.vehicle_action = Mission::MissionItem::VehicleAction::Takeoff;
    missionItems.push_back(item);

    Mission::MissionItem item2{};
    item2.latitude_deg = secondLatitude;
    item2.longitude_deg = currentLongitude;
    item2.relative_altitude_m = 5.0f;
    item2.loiter_time_s = 1.0f;
    item2.acceptance_radius_m = 0.5f;
    item2.yaw_deg = 0;
    missionItems.push_back(item2);

    Mission::MissionItem item3{};
    item3.latitude_deg = thirdLatitude;
    item3.longitude_deg = currentLongitude;
    item3.relative_altitude_m = 5.0f;
    item3.loiter_time_s = 1.0f;
    item3.acceptance_radius_m = 0.5f;
    item3.yaw_deg = 0;
    missionItems.push_back(item3);

    Mission::MissionItem item4{};
    item4.latitude_deg = fourthLatitude;
    item4.longitude_deg = currentLongitude;
    item4.relative_altitude_m = 5.0f;
    item4.loiter_time_s = 1.0f;
    item4.acceptance_radius_m = 0.5f;
    item4.yaw_deg = 0;
    missionItems.push_back(item4);

    Mission::MissionItem item5{};
    item5.latitude_deg = thirdLatitude;
    item5.longitude_deg = currentLongitude;
    item5.relative_altitude_m = 5.0f;
    item5.is_fly_through = true;
    item5.acceptance_radius_m = 0.5f;
    item5.yaw_deg = 180;
    missionItems.push_back(item5);

    Mission::MissionItem item6{};
    item6.latitude_deg = secondLatitude;
    item6.longitude_deg = currentLongitude;
    item6.relative_altitude_m = 5.0f;
    item6.is_fly_through = true;
    item6.acceptance_radius_m = 0.5f;
    item6.yaw_deg = 180;
    missionItems.push_back(item6);

    Mission::MissionItem item7{};
    item7.latitude_deg = firstLatitude;
    item7.longitude_deg = currentLongitude;
    item7.relative_altitude_m = 5.0f;
    item7.acceptance_radius_m = 0.5f;
    item7.yaw_deg = 180;
    item7.vehicle_action = Mission::MissionItem::VehicleAction::Land;
    missionItems.push_back(item7);

    std::cout << "MissionController: Uploading Mission..." << std::endl;

    Mission::MissionPlan plan{};
    plan.mission_items = missionItems;

    if(mission.upload_mission(plan) != Mission::Result::Success) {
        std::cout << "MissionController: Mission Upload Failed." << std::endl;
        if(groundBaseCommunication.joinable()) {
            groundBaseCommunication.join();
        }
        return 1;
    }

    std::cout << "MissionController: Mission Uploaded!" << std::endl;
    std::cout << "MissionController: Arming..." << std::endl;

    if(action.arm() != Action::Result::Success) {
        std::cout << "MissionController: Arm Failed." << std::endl;
        if(groundBaseCommunication.joinable()) {
            groundBaseCommunication.join();
        }
        return 1;
    }

    std::cout << "MissionController: Armed!" << std::endl;
    std::cout << "MissionController: Starting Mission..." << std::endl;

    if(mission.start_mission() != Mission::Result::Success) {
        std::cout << "MissionController: Mission Start Failed." << std::endl;
        if(groundBaseCommunication.joinable()) {
            groundBaseCommunication.join();
        }
        return 1;
    }

    std::cout << "MissionController: Mission Started!" << std::endl;

    while(true) {
        auto missionStatus = mission.is_mission_finished();

        if(missionStatus.first != Mission::Result::Success) {
            std::cout << "MissionController: Mission Status Check Failed." << std::endl;
            break;
        }

        if(missionStatus.second) {
            std::cout << "MissionController: Mission Finished!" << std::endl;
            break;
        }
        else {
            std::cout << "MissionController: Mission Active..." << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Shutting down..." << std::endl;

    communicateWithGroundBase = false;
    if(groundBaseCommunication.joinable()) {
        groundBaseCommunication.join();
    }

    if(internalServer) {
        internalServer->ShutDown();
    }

    if(internalCommunication.joinable()) {
        internalCommunication.join();
    }

    return 0;
}
