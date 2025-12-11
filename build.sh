#!/bin/bash
# Simple build script for LIMP library (alternative to CMake)

CXX=${CXX:-g++}
CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude"
OUTDIR="build_simple"

echo "=== Building LIMP Library ==="
echo "Compiler: $CXX"
echo "Flags: $CXXFLAGS"
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
