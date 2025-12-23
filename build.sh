#!/bin/bash
# LIMP Build Script for Linux/macOS
set -e

PRESET="${1:-release}"
CONAN_BUILD_TYPE="Release"

# Map presets to Conan build types
case "$PRESET" in
  debug|test)
    CONAN_BUILD_TYPE="Debug"
    ;;
esac

echo "==> Installing dependencies with Conan ($CONAN_BUILD_TYPE)..."
conan install . --output-folder=build/$PRESET --build=missing -s build_type=$CONAN_BUILD_TYPE

echo "==> Configuring with CMake (preset: $PRESET)..."
cmake --preset $PRESET

echo "==> Building..."
cmake --build --preset $PRESET

if [ "$PRESET" = "test" ]; then
    echo "==> Running tests..."
    ctest --preset test
fi

echo "==> Build complete! Output in build/$PRESET/"
