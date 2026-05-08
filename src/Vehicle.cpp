#include "Vehicle.h"

#include <memory>
#include <stdexcept>
#include <thread>
#include <iostream>
#include <chrono>

Vehicle::Vehicle() {
    Connect();
    Detect();

    system = mavsdk.systems().at(0);
    action = std::make_unique<mavsdk::Action>(system);
    telemetry = std::make_unique<mavsdk::Telemetry>(system);
    mission = std::make_unique<mavsdk::Mission>(system);

    CheckHealth();
}

void Vehicle::Connect() {
    mavsdk::ConnectionResult connectionResult = mavsdk.add_any_connection("udpin://127.0.0.1:14540");
    if (connectionResult != mavsdk::ConnectionResult::Success) {
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
    while(!telemetry->health_all_ok()) {
        std::cout << "Vehicle::CheckHealth: Vehicle Not Ready To Arm..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Vehicle::CheckHealth: Ready To Arm!" << std::endl;
}

void Vehicle::Kill() {
    if(action->kill() != mavsdk::Action::Result::Success) {
        throw std::runtime_error("Vehicle::Kill: Kill Failed.");
    }
}

void Vehicle::Arm() {
    if(action->arm() != mavsdk::Action::Result::Success) {
        throw std::runtime_error("Vehicle::Arm: Arm Failed.");
    }

    std::cout << "MissionController: Armed!" << std::endl;
}

void Vehicle::StartMission(const std::vector<mavsdk::Mission::MissionItem>& missionItems) {
    if(telemetry->armed()) {
        Hold();
    }

    try {
        ClearMission(); // TODO: Fake, if missions list is empty it returns error
    } catch (const std::exception& error) {
        std::cout << "Vehicle::StartMission: Clear" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    mavsdk::Mission::MissionPlan plan{};
    plan.mission_items = missionItems;

    if(mission->upload_mission(plan) != mavsdk::Mission::Result::Success) {
        throw std::runtime_error("Vehicle::CompleteMission: Mission Upload Failed.");
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if(!telemetry->armed()) { //TODO: Confirmation
        Arm();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    if(mission->start_mission() != mavsdk::Mission::Result::Success) {
        throw std::runtime_error("Vehicle::CompleteMission: Mission Start Failed.");
    }

    std::cout << "Vehicle::CompleteMission: Mission Started!" << std::endl;
}

void Vehicle::ClearMission() {
    if(mission->clear_mission() != mavsdk::Mission::Result::Success) {
        throw std::runtime_error("Vehicle::ClearMission: Failed.");
    }

    std::cout << "Vehicle::ClearMission: Mission Cleared!" << std::endl;
}

TelemetryData Vehicle::GetTelemetry() {
    TelemetryData data{};
    data.latitude_deg = telemetry->position().latitude_deg;
    data.longitude_deg = telemetry->position().longitude_deg;
    data.absolute_altitude_m = telemetry->position().absolute_altitude_m;
    data.relative_altitude_m = telemetry->position().relative_altitude_m;

    data.voltage_v = telemetry->battery().voltage_v;
    data.current_battery_a = telemetry->battery().current_battery_a;
    data.remaining_percent = telemetry->battery().remaining_percent;

    return data;
}

void Vehicle::Hold() {
    if(action->hold() != mavsdk::Action::Result::Success) {
        throw std::runtime_error("Vehicle::Hold: Hold Failed.");
    }

    std::cout << "MissionController: Hold!" << std::endl;
}
