#pragma once

#include <string>
#include <mutex>

#include "Vehicle.h"
#include "CommunicationContext.h"

class ExternalCommunicationImplemenation {
public:
    ExternalCommunicationImplemenation(Vehicle& v, CommunicationContext& c);

    std::string ProccessCommand(const std::string& command);

private:
    std::string Kill();
    std::string Telemetry();
    std::string Prompt(const std::string& command);

private:
    Vehicle& vehicle;
    CommunicationContext& communicationContext;
};
