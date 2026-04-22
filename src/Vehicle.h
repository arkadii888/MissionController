#pragma once

#include "mavsdk/mavsdk.h"
#include "mavsdk/plugins/action/action.h"
#include "mavsdk/plugins/telemetry/telemetry.h"
#include "mavsdk/plugins/mission/mission.h"

#include <memory>

using namespace mavsdk;

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
    void CompleteMission(const std::vector<Mission::MissionItem>& missionItems);
    void ClearMission();

    TelemetryData GetTelemetry();

private:
    void Connect();
    void Detect();
    void CheckHealth();

private:
    Mavsdk mavsdk{Mavsdk::Configuration{ComponentType::GroundStation}};
    std::shared_ptr<System> system;
    std::unique_ptr<Action> action;
    std::unique_ptr<Telemetry> telemetry;
    std::unique_ptr<Mission> mission;
};
