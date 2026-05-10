#include "ExternalCommunicationImplementation.h"
#include <mutex>

ExternalCommunicationImplemenation::ExternalCommunicationImplemenation(Vehicle& v, CommunicationContext& c, MediaContext& m) : vehicle(v), communicationContext(c), mediaContext(m) {

}

std::string ExternalCommunicationImplemenation::ProccessCommand(const std::string& command) {
    if(command == "#kill") {
        return Kill();
    }
    else if(command == "#telemetry") {
        return Telemetry();
    }
    else if(command == "#photo") {
        return Photo();
    }
    else {
        return Prompt(command);
    }
}

std::string ExternalCommunicationImplemenation::Kill() {
    std::string reply = "Killed!";
    try {
        vehicle.Kill();
    } catch (const std::exception& error) {
        reply = error.what();
    }
    return reply;
}

std::string ExternalCommunicationImplemenation::Telemetry() {
    auto telemetry = vehicle.GetTelemetry();

    std::string reply = "{";
    reply += "\"latitude_deg\":" + std::to_string(telemetry.latitude_deg) + ",";
    reply += "\"longitude_deg\":" + std::to_string(telemetry.longitude_deg) + ",";
    reply += "\"absolute_altitude_m\":" + std::to_string(telemetry.absolute_altitude_m) + ",";
    reply += "\"relative_altitude_m\":" + std::to_string(telemetry.relative_altitude_m) + ",";
    reply += "\"voltage_v\":" + std::to_string(telemetry.voltage_v) + ",";
    reply += "\"current_battery_a\":" + std::to_string(telemetry.current_battery_a) + ",";
    reply += "\"remaining_percent\":" + std::to_string(telemetry.remaining_percent);
    reply += "}";

    return reply;
}

std::string ExternalCommunicationImplemenation::Prompt(const std::string& command) {
    std::lock_guard<std::mutex> lock(communicationContext.promptMutex);
    communicationContext.prompt = command;
    return "Mission in progress";
}

std::string ExternalCommunicationImplemenation::Photo() {
    std::unique_lock<std::mutex> lock(mediaContext.photoMutex, std::try_to_lock);
    if(lock.owns_lock()) {
        std::string copy = mediaContext.photo;
        mediaContext.photo = "";
        return copy;
    }

    return "";
}
