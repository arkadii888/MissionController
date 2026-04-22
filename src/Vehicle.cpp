#include "Vehicle.h"

#include <memory>
#include <stdexcept>
#include <thread>
#include <iostream>

Vehicle::Vehicle() {
    Connect();
    Detect();

    system = mavsdk.systems().at(0);
    action = std::make_unique<Action>(system);
    telemetry = std::make_unique<Telemetry>(system);
    mission = std::make_unique<Mission>(system);

    CheckHealth();
}

void Vehicle::Connect() {
    ConnectionResult connectionResult = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");
    if (connectionResult != ConnectionResult::Success) {
        throw std::runtime_error("Vehicle::Connect: Connection Failed.");
    }

    std::cout << "Vehicle::Connect: Vehicle Connected!" << std::endl;
}

void Vehicle::Detect() {
    while(mavsdk.systems().empty()) {
        std::cout << "Vehicle::Detect: Vehicle Not Detected Yet..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Vehicle::Detect: Vehicle Detected!" << std::endl;
}

void Vehicle::CheckHealth() {
    while(!telemetry.health_all_ok()) {
        std::cout << "Vehicle::CheckHealth: Vehicle Not Ready To Arm..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Vehicle::CheckHealth: Ready To Arm!" << std::endl;
}

void Vehicle::Kill() {
    if(action.kill() != Action::Result::Success) {
        throw std::runtime_error("Vehicle::Kill: Kill Failed.");
    }
}

void Vehicle::Arm() {
    if(action.arm() != Action::Result::Success) {
        throw std::runtime_error("Vehicle::Arm: Arm Failed.");
    }

    std::cout << "MissionController: Armed!" << std::endl;
}

void Vehicle::CompleteMission(const std::vector<Mission::MissionItem>& missionItems) {
    Mission::MissionPlan plan{};
    plan.mission_items = missionItems;

    if(mission.upload_mission(plan) != Mission::Result::Success) {
        throw std::runtime_error("Vehicle::CompleteMission: Mission Upload Failed.");
    }

    if(mission.start_mission() != Mission::Result::Success) {
        throw std::runtime_error("Vehicle::CompleteMission: Mission Start Failed.");
    }

    std::cout << "Vehicle::CompleteMission: Mission Started!" << std::endl;

    while(true) {
        auto missionStatus = mission.is_mission_finished();

        if(missionStatus.first != Mission::Result::Success) {
            throw std::runtime_error("Vehicle::CompleteMission: Mission Status Check Failed.");
        }

        if(missionStatus.second) {
            break;
        }

        std::cout << "Vehicle::CompleteMission: Mission Active..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << "Vehicle::CompleteMission: Mission Completed!" << std::endl;
}

void Vehicle::ClearMission() {
    if(mission.clear_mission() != Mission::Result::Success) {
        throw std::runtime_error("Vehicle::ClearMission: Failed.");
    }

    std::cout << "Vehicle::ClearMission: Mission Cleared!" << std::endl;
}

TelemetryData Vehicle::GetTelemetry() {
    TelemetryData data{};
    data.latitude_deg = telemetry.position().latitude_deg;
    data.longitude_deg = telemetry.position().longitude_deg;
    data.absolute_altitude_m = telemetry.position().absolute_altitude_m;
    data.relative_altitude_m = telemetry.position().relative_altitude_m;

    return data;
}
