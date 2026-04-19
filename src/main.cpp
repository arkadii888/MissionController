#include <mavsdk/mavsdk.h>
#include <mavsdk/plugins/action/action.h>
#include <mavsdk/plugins/telemetry/telemetry.h>
#include <iostream>
#include <chrono>
#include <thread>

using namespace mavsdk;
using std::chrono::seconds;
using std::this_thread::sleep_for;

int main() {
    Mavsdk mavsdk{Mavsdk::Configuration{ComponentType::GroundStation}};

    ConnectionResult conn_result = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");

    if (conn_result != ConnectionResult::Success) {
        std::cerr << "Connection failed: " << conn_result << std::endl;
        return 1;
    }

    std::cout << "Waiting for drone..." << std::endl;
    while (mavsdk.systems().empty()) {
        sleep_for(seconds(1));
    }

    auto drone_system = mavsdk.systems().at(0);
    auto action = Action{drone_system};
    auto telemetry = Telemetry{drone_system};

    std::cout << "Checking system health..." << std::endl;
    while (!telemetry.health_all_ok()) {
        sleep_for(seconds(1));
    }

    std::cout << "Arming..." << std::endl;
    auto arm_result = action.arm();
    if (arm_result != Action::Result::Success) {
        std::cerr << "Arming failed: " << arm_result << std::endl;
        return 1;
    }

    std::cout << "Takeoff!" << std::endl;
    auto takeoff_result = action.takeoff();
    if (takeoff_result != Action::Result::Success) {
        std::cerr << "Takeoff failed: " << takeoff_result << std::endl;
        return 1;
    }

    sleep_for(seconds(5));

    std::cout << "Landing..." << std::endl;
    auto land_result = action.land();
    if (land_result != Action::Result::Success) {
        std::cerr << "Landing failed: " << land_result << std::endl;
        return 1;
    }

    return 0;
}
