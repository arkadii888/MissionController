#include "ExternalCommunicationImplementation.h"

ExternalCommunicationImplemenation::ExternalCommunicationImplemenation(Vehicle& v, CommunicationContext& c) : vehicle(v), communicationContext(c) {

}

std::string ExternalCommunicationImplemenation::ProccessCommand(const std::string& command) {
    if(command == "#kill") {
        return Kill();
    }
    else if(command == "#telemetry") {
        return Telemetry();
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
