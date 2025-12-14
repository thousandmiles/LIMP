# LIMP - Lightweight Industrial Messaging Protocol

Modern C++17 library for industrial automation messaging (SCADA, HMI, PLC communication).

## Features

- **Modern C++17** - Type-safe, zero external dependencies (core)
- **Compact Binary Protocol** - 16-byte header + payload, CRC16-MODBUS validation
- **Builder Pattern API** - Fluent interface for message construction
- **Optional ZeroMQ Transport** - High-performance REQ-REP and PUB-SUB patterns
- **Cross-platform** - Linux, Windows, macOS

## Quick Start

### Build

```bash
# Simple build (no dependencies)
./build.sh

# With ZeroMQ support
./build.sh --with-zmq

# Or using CMake
mkdir build && cd build
cmake -DLIMP_BUILD_ZMQ=ON ..
make
```

### ZeroMQ Dependencies (optional)

```bash
# Ubuntu/Debian
sudo apt-get install libzmq3-dev
git clone https://github.com/zeromq/cppzmq.git
cd cppzmq && mkdir build && cd build
cmake .. && sudo make install
```

## Usage

```cpp
#include <limp/limp.hpp>
using namespace limp;

// Build request
auto request = MessageBuilder::request(0x10, 0x30, 0x3000, 7, 1)
    .setPayload(123.45f)
    .build();

std::vector<uint8_t> buffer;
serializeFrame(request, buffer);  // Send buffer...

// Parse response
Frame response;
deserializeFrame(buffer, response);
MessageParser parser(response);
auto value = parser.getFloat32();
```

**ZeroMQ (if enabled):**

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

// Pub-Sub
ZMQPublisher pub;
pub.bind("tcp://0.0.0.0:5556");
pub.publish("topic", data, size);

ZMQSubscriber sub;
sub.connect("tcp://127.0.0.1:5556");
sub.subscribe("topic");
sub.receive(frame);
```

## Protocol Overview

16-byte header + payload (0-65KB) | CRC16-MODBUS | Big-endian | Class/Instance/Attribute addressing

**Types:** REQUEST, RESPONSE, EVENT, ERROR, SUBSCRIBE, UNSUBSCRIBE, ACK

## API

**Core:** `MessageBuilder`, `MessageParser`, `Frame`, `serializeFrame()`, `deserializeFrame()`  
**ZMQ:** `ZMQClient`, `ZMQServer`, `ZMQPublisher`, `ZMQSubscriber`, `ZMQConfig`  
**Enums:** `NodeID`, `ClassID`, `MsgType`, `ErrorCode`

## Integration

**Install ZeroMQ (optional):**

```bash
sudo apt-get install libzmq3-dev
git clone https://github.com/zeromq/cppzmq.git && cd cppzmq
mkdir build && cd build && cmake .. && sudo make install
```

**CMake FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(limp GIT_REPOSITORY https://github.com/yourname/LIMP.git GIT_TAG main)
FetchContent_MakeAvailable(limp)
target_link_libraries(your_app PRIVATE limp)
# set(LIMP_BUILD_ZMQ ON) for ZeroMQ support
```

**Submodule:**

```bash
git submodule add https://github.com/yourname/LIMP.git third_party/limp
```

```cmake
add_subdirectory(third_party/limp)
target_link_libraries(your_app PRIVATE limp)
```

## Notes

Not thread-safe. ZeroMQ sockets: one per thread. Requires C++17 (GCC 7+, Clang 6+, MSVC 2017+).

**Implementation Protection:** Library uses header+source separation. Users receive only public API headers (.hpp) with comprehensive docstrings and compiled binaries - implementation details (.cpp) remain private.

## License

MIT
