#include <chrono>
#include <exception>
#include <iostream>
#include <memory>
#include <thread>
#include <cstring>
#include <atomic>

#include <mavsdk/log_callback.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>

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
    int server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int opt = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8888);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(server, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress));

    listen(server, 5);

    ExternalCommunicationImplemenation e(vehicle, communicationContext, mediaContext);

    while(communicateExternally) {
            sockaddr_in clientAddress;
            socklen_t socklen = sizeof(clientAddress);

            int client = accept(server, reinterpret_cast<sockaddr*>(&clientAddress), &socklen);
            if (client < 0) {
                continue;
            }

            timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 500000;
            setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

            char buffer[256];
            memset(buffer, 0, sizeof(buffer));

            int bytesReceived = recv(client, buffer, sizeof(buffer) - 1, 0);
            if(bytesReceived > 0) {
                buffer[bytesReceived] = '\0';
                std::string command(buffer);
                std::string reply = e.ProccessCommand(command);

                size_t totalSent = 0;
                size_t bytesLeft = reply.length();
                const char* dataPtr = reply.c_str();

                while(totalSent < bytesLeft) {
                    int sent = send(client, dataPtr + totalSent, bytesLeft - totalSent, 0);
                    if (sent == -1) {
                        break;
                    }
                    totalSent += sent;
                }
            }

            close(client);
        }
    close(server);
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
        mediaHandling = std::thread(MediaHandling, std::ref(mediaContext));

        while(true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    } catch (const std::exception& error) {
        std::cout << "Error: " << error.what() << std::endl; //TODO: React - land or return, depends
    }

    ClearThreads(externalCommunication, internalCommunication, mediaHandling);
    return 0;
}
