#pragma once

#include "mavsdk/mavsdk.h"
#include "mavsdk/plugins/action/action.h"
#include "mavsdk/plugins/telemetry/telemetry.h"
#include "mavsdk/plugins/mission/mission.h"

#include <memory>
#include <vector>

struct TelemetryData {
    double latitude_deg = 0.0f;
    double longitude_deg = 0.0f;
    float absolute_altitude_m = 0.0f;
    float relative_altitude_m = 0.0f;
};

class Vehicle {
public:
    Vehicle();

    void Kill();
    void Arm();
    void StartMission(const std::vector<mavsdk::Mission::MissionItem>& missionItems);
    void ClearMission();

    TelemetryData GetTelemetry();

private:
    void Connect();
    void Detect();
    void CheckHealth();

private:
    mavsdk::Mavsdk mavsdk{mavsdk::Mavsdk::Configuration{ComponentType::GroundStation}};
    std::shared_ptr<mavsdk::System> system;
    std::unique_ptr<mavsdk::Action> action;
    std::unique_ptr<mavsdk::Telemetry> telemetry;
    std::unique_ptr<mavsdk::Mission> mission;
};
