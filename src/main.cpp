#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <atomic>

#include <mavsdk/log_callback.h>

// UDP
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>

// gRPC
#include <grpcpp/grpcpp.h>
#include "internal_communication.grpc.pb.h"

#include "Vehicle.h"
#include "CommunicationContext.h"
#include "InternalCommunicationImplementation.h"
#include "ExternalCommunicationImplementation.h"
#include "MediaContext.h"
#include "MediaHandler.h"

std::unique_ptr<grpc::Server> internalServer;

void InternalCommunication(Vehicle& vehicle, CommunicationContext& communicationContext) {
    InternalCommunicationImplementation i(vehicle, communicationContext);
    grpc::ServerBuilder builder;

    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());
    builder.RegisterService(&i);

    internalServer = builder.BuildAndStart();

    internalServer->Wait();
}

std::atomic<bool> communicateExternally = true;

void ExternalCommunication(Vehicle& vehicle, CommunicationContext& communicationContext, MediaContext& mediaContext) {
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);

    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(8888);
    server.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(s, reinterpret_cast<sockaddr*>(&server), sizeof(server));

    char buffer[256];
    ExternalCommunicationImplemenation e(vehicle, communicationContext, mediaContext);

    while(communicateExternally) {
        sockaddr_in client;
        socklen_t socklen = sizeof(client);

        memset(buffer, 0, sizeof(buffer));

        int bytesReceived = recvfrom(s, buffer, sizeof(buffer) - 1, 0, reinterpret_cast<sockaddr*>(&client), &socklen);
        if(bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::string command(buffer);
            std::string reply = e.ProccessCommand(command);
            sendto(s, reply.c_str(), reply.length(), 0, reinterpret_cast<sockaddr*>(&client), socklen);
        }
    }
    close(s);
}

std::atomic<bool> handleMedia = true;

void MediaHandling(MediaContext& mediaContext) {
    MediaHandler handler{mediaContext};

    while(handleMedia) {
        handler.ReadPhoto();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void ClearThreads(std::thread& externalCommunication, std::thread& internalCommunication, std::thread& mediaHandling) {
    handleMedia = false;
    if(mediaHandling.joinable()) {
        mediaHandling.join();
    }

    communicateExternally = false;
    if(externalCommunication.joinable()) {
        externalCommunication.join();
    }

    if(internalServer) {
        internalServer->Shutdown();
    }

    if(internalCommunication.joinable()) {
        internalCommunication.join();
    }
}

int main() {
    mavsdk::log::subscribe([](mavsdk::log::Level level, const std::string& message, const std::string& file, int line) {
        return true;
    });

    std::thread externalCommunication;
    std::thread internalCommunication;
    std::thread mediaHandling;

    try {
        Vehicle vehicle;

        CommunicationContext communicationContext;
        MediaContext mediaContext;
        externalCommunication = std::thread(ExternalCommunication, std::ref(vehicle), std::ref(communicationContext), std::ref(mediaContext));
        internalCommunication = std::thread(InternalCommunication, std::ref(vehicle), std::ref(communicationContext));
        mediaHandling = std::thread(mediaHandling, std::ref(mediaContext));

        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& error) {
        std::cout << "Error: " << error.what() << std::endl; //TODO: React - land or return, depends
    }

    ClearThreads(externalCommunication, internalCommunication, mediaHandling);
    return 0;
}
