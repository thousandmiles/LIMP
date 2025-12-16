/**
 * @file router.cpp
 * @brief Example demonstrating ZeroMQ ROUTER pattern for message routing
 *
 * This example shows how to use the ZMQRouter class to receive messages
 * from multiple DEALER clients and route responses back to specific clients.
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <iomanip>
#include <map>

using namespace limp;

// Signal handling for clean shutdown
std::atomic<bool> running{true};

void signalHandler(int signal)
{
    (void)signal;
    running = false;
}

// Helper function to print identity as hex
std::string identityToString(const std::vector<uint8_t> &identity)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < identity.size() && i < 8; ++i)
    {
        oss << std::setw(2) << static_cast<int>(identity[i]);
    }
    if (identity.size() > 8)
    {
        oss << "...";
    }
    return oss.str();
}

int main()
{
    std::cout << "=== LIMP ZeroMQ Router Example ===" << std::endl;
    std::cout << std::endl;

    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Configure ZeroMQ transport
    ZMQConfig config;
    config.receiveTimeout = 1000; // 1 second receive timeout (for responsive shutdown)
    config.sendTimeout = 3000;    // 3 second send timeout
    config.lingerTime = 1000;     // 1 second linger on close

    // Create router transport
    ZMQRouter router(config);

    // Set error callback
    router.setErrorCallback([](const std::string &errorMsg)
                            { std::cerr << "Router Error: " << errorMsg << std::endl; });

    // Bind to endpoint
    std::string endpoint = "tcp://*:5555";
    std::cout << "Binding router to " << endpoint << "..." << std::endl;

    if (!router.bind(endpoint))
    {
        std::cerr << "Failed to bind to endpoint" << std::endl;
        return 1;
    }

    std::cout << "Router listening on " << endpoint << std::endl;
    std::cout << "Waiting for DEALER clients to connect..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Track connected clients and message counts
    std::map<std::string, uint32_t> clientStats;
    int totalMessages = 0;

    // Main router loop
    while (running)
    {
        // Receive message with client identity
        std::string clientIdentity;
        Frame request;

        if (!router.receive(clientIdentity, request, 1000))
        {
            // Timeout or error - check if we should continue
            continue;
        }

        totalMessages++;

        // Track client statistics
        clientStats[clientIdentity]++;

        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Received message #" << totalMessages << std::endl;
        std::cout << "Client Identity: " << clientIdentity << std::endl;

        // Parse the LIMP message
        MessageParser parser(request);
        uint16_t srcNode = parser.srcNode();
        MsgType msgType = parser.msgType();
        uint16_t classID = parser.classID();
        uint16_t instanceID = parser.instanceID();
        uint16_t attrID = parser.attrID();

        std::cout << "Source Node: 0x" << std::hex << std::setw(4) << std::setfill('0')
                  << srcNode << std::dec << std::endl;
        std::cout << "Message Type: 0x" << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(msgType) << std::dec << std::endl;
        std::cout << "Payload size: " << parser.frame().payloadLen << " bytes" << std::endl;

        // Create response message
        Frame responseFrame = MessageBuilder::response(0x0020, classID, instanceID, attrID)
                                  .build();

        // Route response back to specific client
        if (!router.send(clientIdentity, responseFrame))
        {
            std::cerr << "Failed to send response to client" << std::endl;
        }
        else
        {
            std::cout << "Response routed back to client: " << clientIdentity << std::endl;
        }

        // Print statistics
        std::cout << std::endl
                  << "Statistics:" << std::endl;
        std::cout << "  Total messages: " << totalMessages << std::endl;
        std::cout << "  Active clients: " << clientStats.size() << std::endl;
        for (const auto &[id, count] : clientStats)
        {
            std::cout << "    Client " << id << ": " << count << " messages" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "Shutting down router..." << std::endl;
    router.close();

    std::cout << std::endl;
    std::cout << "Final Statistics:" << std::endl;
    std::cout << "  Total messages processed: " << totalMessages << std::endl;
    std::cout << "  Total clients seen: " << clientStats.size() << std::endl;

    return 0;
}
