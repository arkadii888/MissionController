#include <iostream>
#include "mavsdk/mavsdk.h"

int main() {
    Mavsdk mavsdk{Mavsdk::Configuration{Mavsdk::ComponentType::GroundStation}};
    ConnectionResult connectionResult = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");
    if (connectionResult != ConnectionResult::Success) {
        std::cout << "Adding Connection Failed: " << connectionResult << '\n';
        return 1;
    }

    return 0;
}
