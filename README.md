# Executor

A C++ application that hosts a TCP server for communication with the GroundBase, a gRPC server for communication with the Agent, and utilizes MAVSDK to communicate with the flight controller.

## How does it work?

The entire flow can be divided into three streams: a gRPC server, a TCP server, and the main program loop.

**gRPC Server**
Runs on port ```50051``` and uses this [proto](https://github.com/arkadii888/Finch/blob/main/internal_communication.proto) file.

**TCP Server**
Runs on port ```8888``` and expects the following:

```#kill``` — emergency drone stop

```#telemetry``` — returns telemetry in JSON format

```#photo``` — returns the latest photo taken by the Agent

```Any other text``` — treated as a prompt
