#pragma once

#include <string>
#include <mutex>

#include "Vehicle.h"
#include "CommunicationContext.h"
#include "MediaContext.h"

class ExternalCommunicationImplemenation {
public:
    ExternalCommunicationImplemenation(Vehicle& v, CommunicationContext& c, MediaContext& m);

    std::string ProccessCommand(const std::string& command);

private:
    std::string Kill();
    std::string Telemetry();
    std::string Prompt(const std::string& command);
    std::string Photo();

private:
    Vehicle& vehicle;
    CommunicationContext& communicationContext;
    MediaContext& mediaContext;
};
