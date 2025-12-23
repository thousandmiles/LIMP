cmake_minimum_required(VERSION 3.15)

project(LIMP
    VERSION 0.1.0
    DESCRIPTION "Lightweight Industrial Messaging Protocol"
    LANGUAGES CXX
)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(LIMP_BUILD_EXAMPLES "Build example applications" OFF)
option(LIMP_BUILD_TESTS "Build unit tests" OFF)
option(LIMP_BUILD_SHARED "Build shared library" OFF)
option(LIMP_BUILD_ZMQ "Build with ZeroMQ transport support" ON)

# Platform-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)  # Windows 7+
endif()

# Library type
if(LIMP_BUILD_SHARED)
    set(LIMP_LIBRARY_TYPE SHARED)
else()
    set(LIMP_LIBRARY_TYPE STATIC)
endif()

# Source files
set(LIMP_SOURCES
    src/frame.cpp
    src/message.cpp
    src/transport.cpp
    src/utils.cpp
    src/crc.cpp
)

set(LIMP_HEADERS
    include/limp/types.hpp
    include/limp/frame.hpp
    include/limp/message.hpp
    include/limp/transport.hpp
    include/limp/utils.hpp
    include/limp/crc.hpp
    include/limp/limp.hpp
)

# ZeroMQ support (optional)
if(LIMP_BUILD_ZMQ)
    find_package(cppzmq REQUIRED)
    
    # Add ZMQ sources and headers
    list(APPEND LIMP_SOURCES 
        src/zmq/zmq_transport_base.cpp
        src/zmq/zmq_client.cpp
        src/zmq/zmq_server.cpp
        src/zmq/zmq_publisher.cpp
        src/zmq/zmq_subscriber.cpp
        src/zmq/zmq_router.cpp
        src/zmq/zmq_dealer.cpp
        src/zmq/zmq_proxy.cpp
    )
    list(APPEND LIMP_HEADERS 
        include/limp/zmq/zmq_config.hpp
        include/limp/zmq/zmq_transport_base.hpp
        include/limp/zmq/zmq_client.hpp
        include/limp/zmq/zmq_server.hpp
        include/limp/zmq/zmq_publisher.hpp
        include/limp/zmq/zmq_subscriber.hpp
        include/limp/zmq/zmq_router.hpp
        include/limp/zmq/zmq_dealer.hpp
        include/limp/zmq/zmq_proxy.hpp
        include/limp/zmq/zmq.hpp
    )
    
    # Define ZMQ enabled macro
    add_definitions(-DLIMP_HAS_ZMQ)
endif()

# Create library
add_library(limp ${LIMP_LIBRARY_TYPE} ${LIMP_SOURCES} ${LIMP_HEADERS})

# Set library properties
set_target_properties(limp PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Include directories
target_include_directories(limp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compiler warnings
if(MSVC)
    target_compile_options(limp PRIVATE /W4 /WX /utf-8)
else()
    target_compile_options(limp PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Platform-specific libraries
if(WIN32)
    target_link_libraries(limp PRIVATE ws2_32)
elseif(UNIX)
    target_link_libraries(limp PRIVATE pthread)
endif()

# Link ZeroMQ if enabled
if(LIMP_BUILD_ZMQ)
    # Link against Conan-provided targets
    target_link_libraries(limp PUBLIC cppzmq libsodium::libsodium)
endif()

# Examples
if(LIMP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(LIMP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
