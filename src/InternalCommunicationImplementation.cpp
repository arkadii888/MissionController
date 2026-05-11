#include "InternalCommunicationImplementation.h"

InternalCommunicationImplementation::InternalCommunicationImplementation(Vehicle& v, CommunicationContext& c) : vehicle(v), communicationContext(c) {

}

grpc::Status InternalCommunicationImplementation::GetTelemetry(grpc::ServerContext*, const Empty*, TelemetryResponse* reply) {
    auto data = vehicle.GetTelemetry();
    reply->set_latitude_deg(data.latitude_deg);
    reply->set_longitude_deg(data.longitude_deg);
    reply->set_absolute_altitude_m(data.absolute_altitude_m);
    reply->set_relative_altitude_m(data.relative_altitude_m);
    reply->set_yaw_deg(data.yaw_deg);
    return grpc::Status::OK;
}

grpc::Status InternalCommunicationImplementation::StartMission(grpc::ServerContext* context, const MissionItemList* request, Empty* reply) {
    std::vector<mavsdk::Mission::MissionItem> items;
    items.reserve(request->items_size());

    for(auto& receivedItem : request->items()) {
        mavsdk::Mission::MissionItem item{};

        item.latitude_deg = receivedItem.latitude_deg();
        item.longitude_deg = receivedItem.longitude_deg();
        item.relative_altitude_m = receivedItem.relative_altitude_m();
        item.speed_m_s = receivedItem.speed_m_s();
        item.is_fly_through = receivedItem.is_fly_through();
        item.gimbal_pitch_deg = receivedItem.gimbal_pitch_deg();
        item.gimbal_yaw_deg = receivedItem.gimbal_yaw_deg();
        item.camera_action = static_cast<mavsdk::Mission::MissionItem::CameraAction>(receivedItem.camera_action());
        item.loiter_time_s = receivedItem.loiter_time_s();
        item.camera_photo_interval_s = receivedItem.camera_photo_interval_s();
        item.acceptance_radius_m = receivedItem.acceptance_radius_m();
        item.yaw_deg = receivedItem.yaw_deg();
        item.camera_photo_distance_m = receivedItem.camera_photo_distance_m();
        item.vehicle_action = static_cast<mavsdk::Mission::MissionItem::VehicleAction>(receivedItem.vehicle_action());

        items.push_back(item);
    }

    try {
        vehicle.StartMission(items);
    } catch (const std::exception& error) {
        std::cout << "Error: " << error.what() << std::endl;
    }
    return grpc::Status::OK;
}

grpc::Status InternalCommunicationImplementation::GetPrompt(grpc::ServerContext* context, const Empty* request, PromptResponse* reply) {
    std::lock_guard<std::mutex> lock(communicationContext.promptMutex);
    reply->set_prompt(communicationContext.prompt);
    communicationContext.prompt = "";
    return grpc::Status::OK;
}
