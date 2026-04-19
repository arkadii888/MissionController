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
    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
    ConnectionResult conn_result = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");

    if (conn_result != ConnectionResult::Success) {
        std::cerr << "Connect Error: " << conn_result << std::endl;
        return 1;
    }

    std::cout << "Wait for Drone Connect..." << std::endl;
    while (mavsdk.systems().empty()) {
        sleep_for(seconds(1));
    }

    auto system = mavsdk.systems().at(0);
    auto action = Action{system};
    auto telemetry = Telemetry{system};

    while (!telemetry.health_all_ok()) {
        std::cout << "Wait When Ready..." << std::endl;
        sleep_for(seconds(1));
    }

    std::cout << "Arming..." << std::endl;
    const Action::Result arm_result = action.arm();
    if (arm_result != Action::Result::Success) {
        std::cerr << "Error Arming: " << arm_result << std::endl;
        return 1;
    }

    std::cout << "Takeoff!" << std::endl;
    const Action::Result takeoff_result = action.takeoff();
    if (takeoff_result != Action::Result::Success) {
        std::cerr << "Error Takeoff: " << takeoff_result << std::endl;
        return 1;
    }

    sleep_for(seconds(5));

    std::cout << "Landing..." << std::endl;
    const Action::Result land_result = action.land();
    if (land_result != Action::Result::Success) {
        std::cerr << "Error Landing: " << land_result << std::endl;
        return 1;
    }

    return 0;
}
