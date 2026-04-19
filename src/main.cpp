#include <chrono>
#include <iostream>
#include <thread>
#include "mavsdk/mavsdk.h"

using namespace mavsdk;

int main() {
    Mavsdk mavsdk(Mavsdk::Configuration(ComponentType::GroundStation));

    ConnectionResult connectionResult = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");
    if (connectionResult != ConnectionResult::Success) {
        std::cout << "Adding Connection Failed: " << static_cast<int>(connectionResult) << std::endl;
        return 1;
    }

    while(mavsdk.systems().empty()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
