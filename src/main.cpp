#include <iostream>
#include "mavsdk/mavsdk.h"
//#include "mavsdk/mavsdk.hpp"

using namespace mavsdk;

int main() {
    Mavsdk mavsdk{Mavsdk::Configuration{ComponentType::GroundStation}};
    ConnectionResult connectionResult = mavsdk.add_any_connection("serial:///dev/ttyAMA0:921600");
    if (connectionResult != ConnectionResult::Success) {
        std::cout << "Adding Connection Failed: " << connectionResult << '\n';
        return 1;
    }

    auto systems = mavsdk.systems();
    for(auto s : systems) {
        if(s) {
            std::cout << s->vehicle_type() << std::endl;
        }
    }

    //auto vehicle = mavsdk.systems()[0];
    //std::cout << "Vehicle Type: " << vehicle->vehicle_type() << std::endl;

    return 0;
}
