#include <iostream>
#include "mavsdk/mavsdk.h"

using namespace mavsdk;

int main() {
    Mavsdk mavsdk{Mavsdk::Configuration{ComponentType::GroundStation}};
    ConnectionResult connectionResult = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");
    if (connectionResult != ConnectionResult::Success) {
        std::cout << "Adding Connection Failed: " << static_cast<int>(connectionResult) << '\n';
        return 1;
    }

    auto systems = mavsdk.systems();
    for(auto s : systems) {
        if(s) {
            std::cout << "Vehicle Type: " << static_cast<int>(s->vehicle_type()) << std::endl;
        }
    }

    return 0;
}
