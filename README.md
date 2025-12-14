# LIMP - Lightweight Industrial Messaging Protocol

Modern C++17 library for industrial automation messaging (SCADA, HMI, PLC communication).

**Features:** Compact binary protocol (16-byte header + payload) | Builder pattern API | ZeroMQ transport | CRC16-MODBUS | Cross-platform

## Linux Usage

### Download Pre-built Package

```bash
# From GitHub Releases: https://github.com/thousandmiles/LIMP/releases/
wget https://github.com/thousandmiles/LIMP/releases/download/v0.1.0/limp-v0.1.0-linux-x64.tar.gz
tar xzf limp-v0.1.0-linux-x64.tar.gz
# Extracts to: include/limp/*.hpp + lib/liblimp.a
```

### Build from Source

```bash
# Clone repository
git clone git@github.com:thousandmiles/LIMP.git
cd LIMP

# Prerequisites: pip, gcc, cmake
pip install conan
conan profile detect

# Build library
conan install . --output-folder=. --build=missing -s build_type=Release
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/build/Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
# Outputs: build/liblimp.a
```

### Build with Examples & Tests

```bash
# Add flags to CMake command
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/build/Release/generators/conan_toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release -DLIMP_BUILD_EXAMPLES=ON -DLIMP_BUILD_TESTS=ON
cmake --build build -j

# Run tests
cd build && ctest

# Run examples
./build/examples/zmq_client_example
./build/examples/zmq_server_example
./build/examples/zmq_pubsub_example
```

### Use in Your Project

**Direct compilation:**

```bash
g++ -std=c++17 -I./include myapp.cpp \
    ./lib/liblimp.a -lzmq -lsodium -lpthread -o myapp
```

**CMake (recommended):**

```cmake
# Option 1: Use as subdirectory (handles dependencies automatically)
add_subdirectory(path/to/LIMP)
target_link_libraries(myapp PRIVATE limp)

# Option 2: Link prebuilt library manually
target_include_directories(myapp PRIVATE path/to/LIMP/include)
target_link_libraries(myapp PRIVATE path/to/LIMP/lib/liblimp.a zmq sodium pthread)
```

## Windows Usage

### Download Pre-built Package

```powershell
# From GitHub Releases: https://github.com/thousandmiles/LIMP/releases/
# Download limp-v0.1.0-windows-x64.zip
# Extract to get: include/limp/*.hpp + lib/limp.lib
```

### Build from Source

```powershell
# Clone repository
git clone git@github.com:thousandmiles/LIMP.git
cd LIMP

# Prerequisites: Python, Visual Studio 2022, CMake
# Open "x64 Native Tools Command Prompt for VS 2022"

pip install conan
conan profile detect

# Build library
conan install . --output-folder=. --build=missing -s build_type=Release
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%CD%\build\generators\conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
# Outputs: build/Release/limp.lib
```

### Build with Examples & Tests

```powershell
# Add flags to CMake command
cmake -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE=%CD%\build\generators\conan_toolchain.cmake `
      -DCMAKE_BUILD_TYPE=Release -DLIMP_BUILD_EXAMPLES=ON -DLIMP_BUILD_TESTS=ON
cmake --build build --config Release

# Run tests
cd build
ctest -C Release

# Run examples
.\build\examples\Release\zmq_client_example.exe
.\build\examples\Release\zmq_server_example.exe
.\build\examples\Release\zmq_pubsub_example.exe
```

### Use in Your Project

**Direct compilation:**

```powershell
cl /std:c++17 /I .\include myapp.cpp .\lib\limp.lib ws2_32.lib
# Note: Ensure ZeroMQ and libsodium DLLs are in PATH or same directory
```

**CMake (recommended):**

```cmake
# Option 1: Use as subdirectory (handles dependencies automatically)
add_subdirectory(path/to/LIMP)
target_link_libraries(myapp PRIVATE limp)

# Option 2: Link prebuilt library manually
target_include_directories(myapp PRIVATE path/to/LIMP/include)
target_link_libraries(myapp PRIVATE path/to/LIMP/lib/limp.lib ws2_32)
```

## API Examples

**Core Protocol (Cross-platform):**

```cpp
#include <limp/limp.hpp>
using namespace limp;

// Build and serialize request
auto request = MessageBuilder::request(0x10, 0x30, 0x3000, 7, 1)
    .setPayload(123.45f).build();
std::vector<uint8_t> buffer;
serializeFrame(request, buffer);

// Deserialize and parse response
Frame response;
deserializeFrame(buffer, response);
auto value = MessageParser(response).getFloat32();
```

**ZeroMQ Transport (Cross-platform):**

```cpp
#include <limp/zmq/zmq.hpp>

// Server
ZMQServer server;
server.bind("tcp://0.0.0.0:5555");
Frame req;
if (server.receive(req)) {
    server.send(MessageBuilder::response(req.dstNodeID, req.srcNodeID,
        req.classID, req.instanceID, req.attrID).setPayload("OK").build());
}

// Client
ZMQClient client;
client.connect("tcp://127.0.0.1:5555");
client.send(MessageBuilder::request(0x10, 0x30, 0x3000, 1, 1).build());
Frame resp;
client.receive(resp);
```

## License

MIT
