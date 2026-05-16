# Executor

A C++ application that hosts a TCP server for communication with the GroundBase, a gRPC server for communication with the Agent, and utilizes MAVSDK to communicate with the flight controller.

## How does it work?

The entire flow can be divided into three threads: a gRPC server, a TCP server, and the main program loop.

**gRPC Server**

Runs on port ```50051``` and uses this [proto](https://github.com/arkadii888/Finch/blob/main/internal_communication.proto) file.

**TCP Server**

Runs on port ```8888``` and expects the following:

```#kill``` — emergency drone stop

```#telemetry``` — returns telemetry in JSON format

```#photo``` — returns the latest photo taken by the Agent

```Any other text``` — treated as a prompt

## Getting Started

**Installing libraries:**

```
wget https://github.com/mavlink/MAVSDK/releases/download/v3.17.1/libmavsdk-dev_3.17.1_ubuntu24.04_amd64.deb 
sudo dpkg -i libmavsdk-dev_3.17.1_ubuntu24.04_amd64.deb
```

```
sudo apt install -y build-essential pkg-config
sudo apt install -y libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev
```

**Compile the proto file** (assuming you saved the proto file in the directory above the Executor folder):

```
cd Executor/src 

protoc -I ../.. --cpp_out=. ../../internal_communication.proto

protoc -I ../.. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../../internal_communication.proto
```

**Build:**

```
cd Executor/build
cmake ..
make
```

**Run:**

```
cd Executor/build
./launch
```

When running it alongside the Agent, we launch it using ```taskset -c 0 ./launch``` to run the Executor on a single core, leaving the remaining 3 cores dedicated to the Agent.

If the flight controller is working properly, you will see ```Ready To Arm``` in the console. This means you can now start the Agent and send your prompt.
