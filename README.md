# Mission Controller

## Build
```
wget https://github.com/mavlink/MAVSDK/releases/download/v3.17.1/libmavsdk-dev_3.17.1_ubuntu24.04_amd64.deb 
sudo dpkg -i libmavsdk-dev_3.17.1_ubuntu24.04_amd64.deb
sudo apt install -y libgrpc++-dev protobuf-compiler-grpc libprotobuf-dev
```

```
cd MissionController/build/
cmake..
make
```
