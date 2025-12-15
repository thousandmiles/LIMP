/**
 * @file central_server_proxy.cpp
 * @brief Central server using ZMQProxy with automatic message forwarding
 *
 * This approach provides zero-overhead automatic forwarding. The proxy:
 * - Automatically routes messages between nodes
 * - Zero-copy message forwarding
 * - No application logic needed
 * - Maximum performance
 *
 * Use when you need:
 * - Simple message broker/forwarder
 * - Maximum throughput (no processing overhead)
 * - Transparent message routing
 * - Load balancing (ROUTER-DEALER pattern)
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>

using namespace limp;

std::atomic<bool> running{true};

void signalHandler(int signal)
{
    (void)signal;
    running = false;
}

int main()
{
    std::cout << "=== Central Server (Proxy Approach) ===" << std::endl;
    std::cout << "This server automatically forwards all messages" << std::endl;
    std::cout << std::endl;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Configure ZeroMQ
    ZMQConfig config;
    config.receiveTimeout = 1000;
    config.sendTimeout = 3000;

    // Create ROUTER-ROUTER proxy for message brokering
    ZMQProxy proxy(ZMQProxy::ProxyType::ROUTER_ROUTER, config);

    proxy.setErrorCallback([](const std::string &errorMsg)
                           { std::cerr << "[PROXY] Error: " << errorMsg << std::endl; });

    // Configure proxy endpoints
    // For ROUTER-ROUTER broker, all nodes connect to the same endpoint
    std::string endpoint = "tcp://0.0.0.0:5555";
    
    proxy.setFrontend(endpoint, true);  // All nodes connect here
    proxy.setBackend(endpoint, true);   // Same endpoint for bidirectional routing

    // Optional: Enable message capture for monitoring
    proxy.setCapture("tcp://0.0.0.0:9999");

    std::cout << "Starting proxy server on " << endpoint << std::endl;
    std::cout << "Monitoring on tcp://0.0.0.0:9999" << std::endl;

    if (!proxy.start())
    {
        std::cerr << "Failed to start proxy" << std::endl;
        return 1;
    }

    std::cout << "Proxy running. Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;
    std::cout << "The proxy automatically forwards messages between nodes:" << std::endl;
    std::cout << "  Node A --[sendTo(\"NodeB\", msg)]--> Proxy --> Node B" << std::endl;
    std::cout << "  Node B --[sendTo(\"NodeA\", reply)]--> Proxy --> Node A" << std::endl;
    std::cout << std::endl;

    // Main loop - proxy runs in background, we just wait
    while (running)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // In this approach, the proxy handles everything automatically
        // No message processing happens here
        // Messages are forwarded with zero-copy, maximum performance
    }

    std::cout << "\nShutting down proxy..." << std::endl;
    proxy.stop();

    std::cout << "Proxy stopped" << std::endl;

    return 0;
}
