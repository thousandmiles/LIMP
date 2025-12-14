# LIMP - Lightweight Industrial Messaging Protocol

Modern C++17 library for industrial automation messaging (SCADA, HMI, PLC communication).

**Features:** Compact binary protocol (16-byte header + payload) | Builder pattern API | ZeroMQ transport | CRC16-MODBUS | Cross-platform

## Quick Start

**Download Pre-built Package:**

```bash
# Download from GitHub Releases
tar xzf limp-v0.1.0-linux-x64.tar.gz
# Package contains: include/limp/*.hpp + lib/liblimp.a (with ZeroMQ)
```

**Or Build from Source:**

```bash
pip install conan
conan profile detect
conan install . --output-folder=build --build=missing
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=build/Release/generators/conan_toolchain.cmake
cmake --build .
# Creates: liblimp.a
```

## Usage

**Core Protocol:**

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

**ZeroMQ Transport:**

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

## Integration

**Using Pre-built Package:**

Create `myapp.cpp`:

```cpp
#include <limp/limp.hpp>
using namespace limp;

int main() {
    auto msg = MessageBuilder::request(0x10, 0x30, 0x3000, 1, 1)
        .setPayload(42).build();
    return 0;
}
```

**Option A - Direct compilation:**

```bash
g++ -std=c++17 -I/path/to/limp/include myapp.cpp /path/to/limp/lib/liblimp.a -o myapp
```

**Option B - CMake:**

```cmake
add_executable(myapp myapp.cpp)
target_include_directories(myapp PRIVATE /path/to/limp/include)
target_link_libraries(myapp /path/to/limp/lib/liblimp.a)
```

**Using Built from Source:**

If you built from source, use the same approach but point to your build directory:

- Headers: `LIMP/include/`
- Library: `LIMP/build/liblimp.a`

## License

MIT
