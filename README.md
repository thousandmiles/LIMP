# LIMP - Lightweight Industrial Messaging Protocol

Modern C++17 library for industrial automation messaging (SCADA, HMI, PLC communication).

**Features:** Compact binary protocol (14-byte header + payload) | Builder pattern API | ZeroMQ transport | CRC16-MODBUS | Cross-platform

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
conan install . --output-folder=. --build=missing -s build_type=Release -o "*:shared=False"
cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="$PWD\build\generators\conan_toolchain.cmake" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
# Outputs: build/Release/limp.lib
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

## Development & Testing

### Building Examples

**Linux:**

```bash
conan install . --output-folder=. --build=missing -s build_type=Release
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/build/Release/generators/conan_toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Release -DLIMP_BUILD_EXAMPLES=ON
cmake --build build -j

# Run examples
./build/examples/zmq_client_example
./build/examples/zmq_server_example
./build/examples/zmq_pubsub_example
```

**Windows:**

```powershell
conan install . --output-folder=. --build=missing -s build_type=Release -o "*:shared=False"
cmake -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="$PWD\build\generators\conan_toolchain.cmake" `
      -DCMAKE_BUILD_TYPE=Release -DLIMP_BUILD_EXAMPLES=ON
cmake --build build --config Release

# Run examples
.\build\examples\Release\zmq_client_example.exe
.\build\examples\Release\zmq_server_example.exe
.\build\examples\Release\zmq_pubsub_example.exe
```

### Running Tests

**Note:** Tests require Debug build mode on both Linux and Windows due to optimization-related issues in the test suite.

**Linux:**

```bash
rm -rf build
conan install . --output-folder=. --build=missing -s build_type=Debug
cmake -B build -DCMAKE_TOOLCHAIN_FILE=$PWD/build/Debug/generators/conan_toolchain.cmake \
      -DCMAKE_BUILD_TYPE=Debug -DLIMP_BUILD_TESTS=ON
cmake --build build -j
cd build && ctest --output-on-failure
```

**Windows:**

```powershell
Remove-Item -Recurse -Force build
conan install . --output-folder=. --build=missing -s build_type=Debug -o "*:shared=False"
cmake -B build -G "Visual Studio 17 2022" -A x64 `
      -DCMAKE_TOOLCHAIN_FILE="$PWD\build\generators\conan_toolchain.cmake" `
      -DCMAKE_BUILD_TYPE=Debug -DLIMP_BUILD_TESTS=ON
cmake --build build --config Debug
cd build
ctest -C Debug --output-on-failure
```

## API Examples

[examples](./examples/)

## License

MIT
