#!/bin/bash
# Simple build script for LIMP library (alternative to CMake)

CXX=${CXX:-g++}
CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude"
OUTDIR="build_simple"
BUILD_ZMQ=0

# Parse command line options
while [[ $# -gt 0 ]]; do
    case $1 in
        --with-zmq)
            BUILD_ZMQ=1
            shift
            ;;
        --help)
            echo "Usage: $0 [--with-zmq] [--help]"
            echo ""
            echo "Options:"
            echo "  --with-zmq    Build with ZeroMQ transport support"
            echo "  --help        Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

echo "=== Building LIMP Library ==="
echo "Compiler: $CXX"
echo "Flags: $CXXFLAGS"
if [ $BUILD_ZMQ -eq 1 ]; then
    echo "ZeroMQ support: ENABLED"
    CXXFLAGS="$CXXFLAGS -DLIMP_HAS_ZMQ"
else
    echo "ZeroMQ support: DISABLED (use --with-zmq to enable)"
fi
echo ""

# Create output directory
mkdir -p $OUTDIR

# Compile library source files
echo "Compiling library..."
$CXX $CXXFLAGS -c src/utils.cpp -o $OUTDIR/utils.o
$CXX $CXXFLAGS -c src/crc.cpp -o $OUTDIR/crc.o
$CXX $CXXFLAGS -c src/frame.cpp -o $OUTDIR/frame.o
$CXX $CXXFLAGS -c src/message.cpp -o $OUTDIR/message.o
$CXX $CXXFLAGS -c src/transport.cpp -o $OUTDIR/transport.o

# Compile ZMQ transport if enabled
if [ $BUILD_ZMQ -eq 1 ]; then
    echo "Compiling ZeroMQ transport..."
    $CXX $CXXFLAGS -c src/zmq/zmq_transport_base.cpp -o $OUTDIR/zmq_transport_base.o
    $CXX $CXXFLAGS -c src/zmq/zmq_client.cpp -o $OUTDIR/zmq_client.o
    $CXX $CXXFLAGS -c src/zmq/zmq_server.cpp -o $OUTDIR/zmq_server.o
    $CXX $CXXFLAGS -c src/zmq/zmq_publisher.cpp -o $OUTDIR/zmq_publisher.o
    $CXX $CXXFLAGS -c src/zmq/zmq_subscriber.cpp -o $OUTDIR/zmq_subscriber.o
fi

# Create static library
echo "Creating static library..."
ar rcs $OUTDIR/liblimp.a $OUTDIR/*.o

echo ""
echo "Library built: $OUTDIR/liblimp.a"
echo ""

# Build examples
echo "Building examples..."
$CXX $CXXFLAGS examples/simple_request.cpp -L$OUTDIR -llimp -o $OUTDIR/simple_request
$CXX $CXXFLAGS examples/simple_response.cpp -L$OUTDIR -llimp -o $OUTDIR/simple_response
$CXX $CXXFLAGS examples/subscribe_example.cpp -L$OUTDIR -llimp -o $OUTDIR/subscribe_example

# Build ZMQ examples if enabled
if [ $BUILD_ZMQ -eq 1 ]; then
    echo "Building ZeroMQ examples..."
    $CXX $CXXFLAGS examples/zmq_client_example.cpp -L$OUTDIR -llimp -lzmq -o $OUTDIR/zmq_client_example
    $CXX $CXXFLAGS examples/zmq_server_example.cpp -L$OUTDIR -llimp -lzmq -o $OUTDIR/zmq_server_example
    $CXX $CXXFLAGS examples/zmq_pubsub_example.cpp -L$OUTDIR -llimp -lzmq -o $OUTDIR/zmq_pubsub_example
fi

echo "Examples built in $OUTDIR/"
echo ""

# Build tests
echo "Building tests..."
$CXX $CXXFLAGS tests/test_frame.cpp -L$OUTDIR -llimp -o $OUTDIR/test_frame

echo "Tests built in $OUTDIR/"
echo ""

echo "=== Build Complete ==="
echo ""
echo "To run examples:"
echo "  ./$OUTDIR/simple_request"
echo "  ./$OUTDIR/simple_response"
echo "  ./$OUTDIR/subscribe_example"
echo ""
echo "To run tests:"
echo "  ./$OUTDIR/test_frame"
