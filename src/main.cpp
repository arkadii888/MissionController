#include <chrono>
#include <iostream>
#include <thread>
#include "mavsdk/mavsdk.h"
#include "mavsdk/plugins/action/action.h"
#include "mavsdk/plugins/telemetry/telemetry.h"

using namespace mavsdk;

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
        std::cout << "MissionController: Vehicle Not Detected Yet." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Vehicle Detected!" << std::endl;

    auto system = mavsdk.systems()[0];
    auto action = Action(system);
    auto telemetry = Telemetry(system);

    std::cout << "MissionController: Checking Health..." << std::endl;

    while(!telemetry.health_all_ok()) {
        std::cout << "MissionController: Vehicle Not Ready To Arm." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Ready To Arm!" << std::endl;
    std::cout << "MissionController: Arming..." << std::endl;

    auto armResult = action.arm();
    if(armResult != Action::Result::Success) {
        std::cout << "MissionController: Arm Failed." << std::endl;
        return 1;
    }

    std::cout << "MissionController: Armed!" << std::endl;
    std::cout << "MissionController: Taking Off..." << std::endl;

    action.set_takeoff_altitude(5.0);
    auto takeoffResult = action.takeoff();
    if(takeoffResult != Action::Result::Success) {
        std::cout << "MissionController: Takeoff Failed." << std::endl;
        return 1;
    }

    std::cout << "MissionController: Took Off!" << std::endl;
    std::cout << "MissionController: Waiting For Correct Altitude..." << std::endl;

    float targetAltitude = action.get_takeoff_altitude().second;
    float currentAltitude = 0.0f;
    while(currentAltitude < targetAltitude) {
        currentAltitude = telemetry.position().relative_altitude_m;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Altitude Reached!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "MissionController: Moving To Target Point..." << std::endl;

    double currentLatitude = telemetry.position().latitude_deg;
    double currentLongitude = telemetry.position().longitude_deg;
    float absoluteAltitude = telemetry.position().absolute_altitude_m;

    double targetLatitude = currentLatitude + 20.0f / 111111.0f;

    auto goResult = action.goto_location(targetLatitude, currentLongitude, absoluteAltitude, 0.0f);
    if(goResult != Action::Result::Success) {
        std::cout << "MissionController: Move Failed." << std::endl;
        return 1;
    }

    std::cout << "MissionController: Move Started!" << std::endl;
    std::cout << "MissionController: Waiting For Correct Position..." << std::endl;

    while(currentLatitude < targetLatitude) {
        currentLatitude = telemetry.position().latitude_deg;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Position Reached!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "MissionController: Moving To Target Point..." << std::endl;

    currentLatitude = telemetry.position().latitude_deg;
    currentLongitude = telemetry.position().longitude_deg;
    absoluteAltitude = telemetry.position().absolute_altitude_m;
    targetLatitude = currentLatitude - 20.0f / 111111.0f;

    goResult = action.goto_location(targetLatitude, currentLongitude, absoluteAltitude, 180.0f);
    if(goResult != Action::Result::Success) {
        std::cout << "MissionController: Move Failed." << std::endl;
        return 1;
    }

    std::cout << "MissionController: Move Started!" << std::endl;
    std::cout << "MissionController: Waiting For Correct Position..." << std::endl;

    while(currentLatitude > targetLatitude) {
        currentLatitude = telemetry.position().latitude_deg;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Position Reached!" << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "MissionController: Landing..." << std::endl;

    auto landResult = action.land();
    if(landResult != Action::Result::Success) {
        std::cout << "MissionController: Land Failed." << std::endl;
        return 1;
    }

    std::cout << "MissionController: Land Started!" << std::endl;
    std::cout << "MissionController: Waiting To Disarm..." << std::endl;

    while(telemetry.armed()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "MissionController: Landed, Disarmed!" << std::endl;

    return 0;
}
