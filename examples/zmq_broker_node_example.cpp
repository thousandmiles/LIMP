/**
 * @file zmq_broker_node_example.cpp
 * @brief Example showing multiple nodes communicating through a central broker
 *
 * This example demonstrates a complete system with a central broker and
 * multiple nodes (HMI, PLC, Logger) communicating through it.
 *
 * Run this alongside zmq_proxy_example.cpp to see the full system in action:
 * 1. Terminal 1: Run zmq_proxy_example (the broker)
 * 2. Terminal 2+: Run this program multiple times with different NODE_TYPE
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace limp;

// Signal handling for clean shutdown
std::atomic<bool> running{true};

void signalHandler(int signal)
{
    (void)signal;
    running = false;
}

// Get node type from environment or default to HMI
std::string getNodeType()
{
    const char *envType = std::getenv("NODE_TYPE");
    return envType ? std::string(envType) : "HMI";
}

int main()
{
    std::cout << "=== LIMP Broker Node Example ===" << std::endl;
    std::cout << std::endl;

    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Determine node type and configuration
    std::string nodeType = getNodeType();
    std::string identity;
    uint16_t nodeID;

    if (nodeType == "PLC")
    {
        identity = "PLC-001";
        nodeID = 0x0030; // NodeID::PLC
    }
    else if (nodeType == "LOGGER")
    {
        identity = "LOGGER-001";
        nodeID = 0x0050; // NodeID::Logger
    }
    else
    {
        // Default to HMI
        nodeType = "HMI";
        identity = "HMI-001";
        nodeID = 0x0010; // NodeID::HMI
    }

    std::cout << "Node Type: " << nodeType << std::endl;
    std::cout << "Identity:  " << identity << std::endl;
    std::cout << "Node ID:   0x" << std::hex << nodeID << std::dec << std::endl;
    std::cout << std::endl;

    // Configure ZeroMQ transport
    ZMQConfig config;
    config.receiveTimeout = 2000; // 2 second timeout
    config.sendTimeout = 3000;
    config.lingerTime = 1000;

    // Create dealer to connect to broker
    ZMQDealer dealer(config);

    // Set error callback
    dealer.setErrorCallback([&nodeType](const std::string &errorMsg)
                            { std::cerr << "[" << nodeType << "] Error: " << errorMsg << std::endl; });

    // Set identity
    dealer.setIdentity(identity);

    // Connect to broker
    std::string brokerEndpoint = "tcp://127.0.0.1:5555";
    std::cout << "Connecting to broker at " << brokerEndpoint << "..." << std::endl;

    if (!dealer.connect(brokerEndpoint))
    {
        std::cerr << "Failed to connect to broker" << std::endl;
        return 1;
    }

    std::cout << "Connected to broker" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Give broker time to register
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    int sentCount = 0;
    int receivedCount = 0;

    // Main communication loop
    while (running)
    {
        // Different behavior based on node type
        if (nodeType == "HMI")
        {
            // HMI sends commands to PLC
            sentCount++;
            std::cout << "[HMI] Sending command #" << sentCount << " to PLC..." << std::endl;

            Frame cmd = MessageBuilder::request(0x0010, 0x2000, 0x0001, 0x0001).build();

            dealer.send(cmd);

            // Try to receive response
            Frame response;
            if (dealer.receive(response, 3000))
            {
                receivedCount++;
                MessageParser parser(response);
                std::cout << "[HMI] Received response from broker (source: 0x"
                          << std::hex << parser.srcNode() << std::dec << ")" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        else if (nodeType == "PLC")
        {
            // PLC waits for commands and responds
            std::cout << "[PLC] Waiting for commands..." << std::endl;

            Frame request;
            if (dealer.receive(request, 5000))
            {
                receivedCount++;
                MessageParser parser(request);
                std::cout << "[PLC] Received command from broker (source: 0x"
                          << std::hex << parser.srcNode() << std::dec << ")" << std::endl;

                // Send response back
                sentCount++;
                Frame response = MessageBuilder::response(0x0030, parser.classID(), 
                                                         parser.instanceID(), 
                                                         parser.attrID()).build();

                dealer.send(response);
                std::cout << "[PLC] Response sent back through broker" << std::endl;
            }
        }
        else if (nodeType == "LOGGER")
        {
            // Logger just listens to all traffic (in real scenario, would need multicast)
            std::cout << "[LOGGER] Monitoring messages..." << std::endl;

            Frame frame;
            if (dealer.receive(frame, 5000))
            {
                receivedCount++;
                MessageParser parser(frame);
                std::cout << "[LOGGER] Message: src=0x" << std::hex << parser.srcNode()
                          << " type=0x" << static_cast<int>(parser.msgType())
                          << std::dec << std::endl;
            }
        }

        std::cout << "  Stats: Sent=" << sentCount << " Received=" << receivedCount << std::endl;
        std::cout << std::endl;
    }

    std::cout << "Shutting down " << nodeType << " node..." << std::endl;
    dealer.close();

    std::cout << "Final Stats: Sent=" << sentCount << " Received=" << receivedCount << std::endl;

    return 0;
}
