#pragma once

#include <vector>
#include <mutex>
#include <iostream>

#include "mavsdk/plugins/mission/mission.h"

#include <grpcpp/grpcpp.h>
#include "internal_communication.grpc.pb.h"

#include "Vehicle.h"
#include "CommunicationContext.h"

class InternalCommunicationImplementation final : public InternalService::Service {
public:
    InternalCommunicationImplementation(Vehicle& v, CommunicationContext& c);

    grpc::Status GetTelemetry(grpc::ServerContext*, const Empty*, TelemetryResponse* reply) override;
    grpc::Status StartMission(grpc::ServerContext* context, const MissionItemList* request, Empty* reply) override;
    grpc::Status GetPrompt(grpc::ServerContext* context, const Empty* request, PromptResponse* reply) override;

private:
    Vehicle& vehicle;
    CommunicationContext& communicationContext;
};
