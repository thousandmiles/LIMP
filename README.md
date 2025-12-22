# LIMP - Lightweight Industrial Messaging Protocol

Modern C++17 library for industrial automation messaging (SCADA, HMI, PLC communication).

**Features:** Compact binary protocol (14-byte header + payload) | Builder pattern API | ZeroMQ transport | CRC16-MODBUS | Cross-platform

**ZeroMQ Patterns:** Client/Server (REQ/REP) | Pub/Sub | Router/Dealer | Proxy (load balancing, message broker, forwarding)

## Linux Usage

### Download Pre-built Package

```bash
# From GitHub Releases: https://github.com/thousandmiles/LIMP/releases/
wget https://github.com/thousandmiles/LIMP/releases/download/v0.1.0/limp-v0.1.0-linux-x64.tar.gz
tar xzf limp-v0.1.0-linux-x64.tar.gz
# Extracts to: include/limp/*.hpp + lib/liblimp.a
```

### Build from Source

**Prerequisites:** Python (pip), GCC/Clang, CMake 3.23+

```bash
# Clone repository
git clone git@github.com:thousandmiles/LIMP.git
cd LIMP

# Install Conan (once)
pip install conan
conan profile detect

# Build library only (outputs: build/liblimp.a)
./build.sh release

# Or build with examples
./build.sh release-all

# Or build and run tests
./build.sh test
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

**Prerequisites:** Python, Visual Studio 2022, CMake 3.23+

```powershell
# Clone repository
git clone git@github.com:thousandmiles/LIMP.git
cd LIMP

# Open "x64 Native Tools Command Prompt for VS 2022"

# Install Conan (once)
pip install conan
conan profile detect

# Build library only (outputs: build/limp.lib)
.\build.ps1 release

# Or build with examples
.\build.ps1 release-all

# Or build and run tests
.\build.ps1 test
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

## Documentation

- **[Transport API Reference](docs/transport_api.md)** - Complete API documentation for all transport classes (Client/Server, Pub/Sub, Router/Dealer)
- **[Router/Dealer API](docs/router_dealer_api.md)** - Detailed guide for advanced routing patterns

## Examples

Comprehensive examples demonstrating all features:

- **[01_basic/](examples/01_basic/)** - Basic LIMP message construction (no ZMQ)
- **[02_request_reply/](examples/02_request_reply/)** - REQ-REP client-server pattern
- **[03_pubsub/](examples/03_pubsub/)** - PUB-SUB broadcasting pattern
- **[04_router_dealer/](examples/04_router_dealer/)** - Advanced async routing
- **[05_broker/](examples/05_broker/)** - Message broker for N:N communication

Each category includes detailed README files with usage instructions, code walkthroughs, and best practices.

See [examples/README.md](examples/README.md) for complete documentation.

## License

MIT
