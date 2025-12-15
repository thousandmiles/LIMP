/**
 * @file zmq_proxy_example.cpp
 * @brief Example demonstrating ZeroMQ Proxy for message brokering
 *
 * This example shows how to use the ZMQProxy class to create a central
 * message broker that forwards messages between multiple nodes.
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace limp;

// Signal handling for clean shutdown
std::atomic<bool> running{true};

void signalHandler(int signal)
{
    (void)signal;
    running = false;
}

int main()
{
    std::cout << "=== LIMP ZeroMQ Proxy/Broker Example ===" << std::endl;
    std::cout << std::endl;

    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Configure ZeroMQ transport
    ZMQConfig config;
    config.receiveTimeout = 1000; // 1 second timeout
    config.sendTimeout = 1000;
    config.lingerTime = 1000;

    // Create a ROUTER-ROUTER proxy for message brokering
    ZMQProxy broker(ZMQProxy::ProxyType::ROUTER_ROUTER, config);

    // Set error callback
    broker.setErrorCallback([](const std::string &errorMsg)
                            { std::cerr << "Broker Error: " << errorMsg << std::endl; });

    // Configure frontend endpoint (clients connect here)
    std::string frontendEndpoint = "tcp://*:5555";
    if (!broker.setFrontend(frontendEndpoint, true))
    {
        std::cerr << "Failed to set frontend endpoint" << std::endl;
        return 1;
    }

    // Configure backend endpoint (can be same as frontend for simple broker)
    // For this example, we use the same endpoint - all nodes connect to 5555
    std::string backendEndpoint = "tcp://*:5555";
    if (!broker.setBackend(backendEndpoint, true))
    {
        std::cerr << "Failed to set backend endpoint" << std::endl;
        return 1;
    }

    // Optional: Set capture endpoint for monitoring
    std::string captureEndpoint = "tcp://*:9999";
    if (!broker.setCapture(captureEndpoint))
    {
        std::cerr << "Failed to set capture endpoint" << std::endl;
        return 1;
    }

    std::cout << "Broker Configuration:" << std::endl;
    std::cout << "  Frontend: " << frontendEndpoint << " (bind)" << std::endl;
    std::cout << "  Backend:  " << backendEndpoint << " (bind)" << std::endl;
    std::cout << "  Capture:  " << captureEndpoint << " (monitoring)" << std::endl;
    std::cout << std::endl;

    // Start the proxy (runs in background thread)
    std::cout << "Starting message broker..." << std::endl;
    if (!broker.start())
    {
        std::cerr << "Failed to start broker" << std::endl;
        return 1;
    }

    std::cout << "Broker is running!" << std::endl;
    std::cout << "Nodes can now connect and communicate through the broker" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Keep running until interrupted
    while (running && broker.isRunning())
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Display status
        static int counter = 0;
        if (++counter % 10 == 0)
        {
            std::cout << "Broker status: Running (" << counter << "s)" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "Shutting down broker..." << std::endl;
    broker.stop();

    std::cout << "Broker stopped cleanly" << std::endl;

    return 0;
}
