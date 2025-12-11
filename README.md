# LIMP - Lightweight Industrial Messaging Protocol

A modern C++17 library implementing the Lightweight Industrial Messaging Protocol (LIMP), designed for industrial automation, SCADA, HMI, and PLC communication systems.

## Features

- 🚀 **Modern C++17** - Type-safe, header-friendly design with clean API
- 🔧 **Cross-platform** - Builds on Linux, Windows, and macOS
- 📦 **Zero external dependencies** - Pure C++ standard library
- ⚡ **High performance** - Efficient binary serialization with minimal overhead
- 🛡️ **CRC16 validation** - Optional data integrity checking
- 🎯 **Builder pattern API** - Intuitive fluent interface for message construction
- 🔌 **Transport agnostic** - Abstract interface for TCP, UDP, Serial, etc.
- 📊 **Complete type support** - UINT8/16/32/64, FLOAT32/64, STRING, OPAQUE

## Protocol Overview

LIMP is a compact binary protocol for industrial messaging with:

- **16-byte fixed header** with optional payload
- **Request/Response, Event, Subscribe/Unsubscribe** message types
- **Class/Instance/Attribute** addressing model
- **Big-endian (network byte order)** encoding
- **Optional CRC16-IBM** error detection
- **Supports up to 65KB payloads**

## Quick Start

### Building the Library

**Option 1: Simple Build Script (no CMake required)**

```bash
# Build everything
./build.sh

# Run tests
./build_simple/test_frame

# Run examples
./build_simple/simple_request
./build_simple/simple_response
./build_simple/subscribe_example
```

**Option 2: CMake (if available)**

```bash
mkdir build && cd build
cmake ..
make

# Run tests
./tests/test_frame
# or: ctest

# Run examples
./examples/simple_request
./examples/simple_response
./examples/subscribe_example
```

### CMake Options

```bash
cmake -DLIMP_BUILD_EXAMPLES=ON \
      -DLIMP_BUILD_TESTS=ON \
      -DLIMP_BUILD_SHARED=OFF \
      ..
```

## Usage Examples

### Creating a Request

```cpp
#include <limp/limp.hpp>

using namespace limp;

// HMI requests Tag[7].Value from PLC
auto request = MessageBuilder::request(
    static_cast<uint16_t>(NodeID::HMI),
    static_cast<uint16_t>(NodeID::PLC),
    static_cast<uint16_t>(ClassID::Tag),
    7,                    // Instance ID
    TagAttr::Value        // Attribute ID
);

Frame frame = request.build();

// Serialize to binary
std::vector<uint8_t> buffer;
serializeFrame(frame, buffer);
// Send buffer over network...
```

### Creating a Response

```cpp
// PLC responds with float32 value
auto response = MessageBuilder::response(
    static_cast<uint16_t>(NodeID::PLC),
    static_cast<uint16_t>(NodeID::HMI),
    static_cast<uint16_t>(ClassID::Tag),
    7,
    TagAttr::Value
)
.setPayload(123.45f)
.enableCRC(true);

Frame frame = response.build();

// Serialize and send...
```

### Parsing a Message

```cpp
// Deserialize received data
Frame frame;
deserializeFrame(buffer, frame);

// Parse payload
MessageParser parser(frame);

if (parser.isResponse()) {
    if (auto value = parser.getFloat32()) {
        std::cout << "Tag value: " << *value << "\n";
    }
}

if (parser.isError()) {
    if (auto code = parser.getErrorCode()) {
        std::cout << "Error: " << toString(*code) << "\n";
    }
}
```

### Subscribe/Event Pattern

```cpp
// Client subscribes to tag updates
auto subscribe = MessageBuilder::subscribe(
    static_cast<uint16_t>(NodeID::HMI),
    static_cast<uint16_t>(NodeID::PLC),
    static_cast<uint16_t>(ClassID::Tag),
    7,
    TagAttr::Value
).build();

// Server sends event when value changes
auto event = MessageBuilder::event(
    static_cast<uint16_t>(NodeID::PLC),
    static_cast<uint16_t>(NodeID::HMI),
    static_cast<uint16_t>(ClassID::Tag),
    7,
    TagAttr::Value
)
.setPayload(newValue)
.enableCRC();
```

### All Payload Types

```cpp
// UINT8, UINT16, UINT32, UINT64
builder.setPayload(static_cast<uint8_t>(42));
builder.setPayload(static_cast<uint32_t>(12345));

// FLOAT32, FLOAT64
builder.setPayload(3.14f);
builder.setPayload(2.71828);

// STRING
builder.setPayload("Temperature alarm");

// OPAQUE (binary data)
std::vector<uint8_t> data = {0xDE, 0xAD, 0xBE, 0xEF};
builder.setPayload(data);
```

## API Reference

### Core Classes

#### `MessageBuilder`

Fluent API for constructing LIMP messages:

- `request()`, `response()`, `event()`, `error()` - Static factory methods
- `subscribe()`, `unsubscribe()`, `ack()` - Subscription management
- `setPayload()` - Set typed payload (overloaded for all types)
- `enableCRC()` - Enable CRC16 validation
- `build()` - Generate final `Frame`

#### `MessageParser`

Extract typed data from frames:

- `getUInt8()`, `getUInt16()`, `getUInt32()`, `getUInt64()`
- `getFloat32()`, `getFloat64()`
- `getString()`, `getOpaque()`
- `getErrorCode()` - Extract error from ERROR messages
- `isRequest()`, `isResponse()`, `isEvent()`, `isError()`

#### `Frame`

Low-level frame structure:

- `serializeFrame()` - Encode frame to binary
- `deserializeFrame()` - Decode binary to frame
- `validate()` - Check frame integrity
- `totalSize()` - Calculate wire size

#### `Transport` (Abstract Interface)

Implement for your transport layer:

```cpp
class MyTransport : public limp::Transport {
    bool send(const Frame& frame) override;
    bool receive(Frame& frame, int timeoutMs) override;
    bool isConnected() const override;
    void close() override;
};
```

### Protocol Types

#### Node IDs

- `NodeID::HMI` (0x0010)
- `NodeID::Server` (0x0020)
- `NodeID::PLC` (0x0030)
- `NodeID::Alarm` (0x0040)
- `NodeID::Logger` (0x0050)
- `NodeID::Broadcast` (0xFFFF)

#### Message Types

- `MsgType::REQUEST`, `RESPONSE`, `EVENT`, `ERROR`
- `MsgType::SUBSCRIBE`, `UNSUBSCRIBE`, `ACK`

#### Class IDs

- `ClassID::System` (0x1000)
- `ClassID::IO` (0x2000)
- `ClassID::Tag` (0x3000)
- `ClassID::Motion` (0x4000)
- `ClassID::AlarmObject` (0x5000)
- `ClassID::LoggerObject` (0x6000)

## Project Structure

```
LIMP/
├── include/limp/        # Public headers
│   ├── types.hpp        # Protocol constants and enums
│   ├── frame.hpp        # Frame serialization
│   ├── message.hpp      # Builder and parser
│   ├── transport.hpp    # Transport interface
│   ├── utils.hpp        # Endianness helpers
│   ├── crc.hpp          # CRC16 calculation
│   └── limp.hpp         # Main include
├── src/                 # Implementation
├── examples/            # Usage examples
├── tests/               # Unit tests
├── CMakeLists.txt       # Build configuration
└── README.md
```

## Installation

### Using CMake (Install)

```bash
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
make
sudo make install
```

### Using CMake (FetchContent)

```cmake
include(FetchContent)
FetchContent_Declare(
    limp
    GIT_REPOSITORY https://github.com/thousandmiles/LIMP.git
    GIT_TAG main
)
FetchContent_MakeAvailable(limp)

target_link_libraries(your_target PRIVATE limp)
```

### Manual Integration

Copy `include/limp/` and `src/` into your project and add to your build system.

## Platform Support

- **Linux** - GCC 7+, Clang 6+
- **Windows** - MSVC 2017+, MinGW
- **macOS** - Xcode 10+, Clang 6+

Requires C++17 compiler.

## Performance Considerations

- **Zero-copy deserialization** possible with raw buffer API
- **Small footprint**: ~16 bytes overhead per message
- **CRC overhead**: 2 bytes + computation time (optional)
- **Header-only option**: Define `LIMP_HEADER_ONLY` for inline implementation

## Thread Safety

The library is **not thread-safe** by design for performance. Use external synchronization if sharing objects across threads.

## Version History

- **0.1.0** - Initial release
  - Core protocol implementation
  - Builder/Parser API
  - CRC16 validation
  - Cross-platform support
