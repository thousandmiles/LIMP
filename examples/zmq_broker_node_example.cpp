/**
 * @file zmq_broker_node_example.cpp
 * @brief Example showing multiple nodes communicating through a central broker
 *
 * This example demonstrates node-to-node communication through a ROUTER-ROUTER
 * proxy broker. Messages are routed by destination identity.
 *
 * Message Flow:
 *   HMI-001 --[sendTo("PLC-001", msg)]--> Broker --[forward]--> PLC-001
 *   PLC-001 --[sendTo("HMI-001", reply)]--> Broker --[forward]--> HMI-001
 *
 * Run this alongside zmq_proxy_example.cpp:
 * 1. Terminal 1: ./zmq_proxy_example (starts the broker)
 * 2. Terminal 2: NODE_TYPE=PLC ./zmq_broker_node_example (PLC node)
 * 3. Terminal 3: NODE_TYPE=HMI ./zmq_broker_node_example (HMI node)
 * 4. Terminal 4: NODE_TYPE=LOGGER ./zmq_broker_node_example (Logger node)
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
            // HMI sends commands to PLC through broker
            sentCount++;
            std::cout << "[HMI] Sending command #" << sentCount << " to PLC-001..." << std::endl;

            Frame cmd = MessageBuilder::request(0x0010, 0x2000, 0x0001, 0x0001).build();

            // Send to specific destination node through broker
            dealer.sendTo("PLC-001", cmd);

            // Try to receive response from PLC
            std::string sourceIdentity;
            Frame response;
            if (dealer.receiveFrom(sourceIdentity, response, 3000))
            {
                receivedCount++;
                MessageParser parser(response);
                std::cout << "[HMI] Received response from " << sourceIdentity 
                          << " (node: 0x" << std::hex << parser.srcNode() << std::dec << ")" << std::endl;
            }
            else
            {
                std::cout << "[HMI] No response received (timeout)" << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        else if (nodeType == "PLC")
        {
            // PLC waits for commands and responds
            std::cout << "[PLC] Waiting for commands..." << std::endl;

            std::string sourceIdentity;
            Frame request;
            if (dealer.receiveFrom(sourceIdentity, request, 5000))
            {
                receivedCount++;
                MessageParser parser(request);
                std::cout << "[PLC] Received command from " << sourceIdentity
                          << " (node: 0x" << std::hex << parser.srcNode() << std::dec << ")" << std::endl;

                // Send response back to the sender through broker
                sentCount++;
                Frame response = MessageBuilder::response(0x0030, parser.classID(), 
                                                         parser.instanceID(), 
                                                         parser.attrID()).build();

                dealer.sendTo(sourceIdentity, response);
                std::cout << "[PLC] Response sent back to " << sourceIdentity << std::endl;
            }
        }
        else if (nodeType == "LOGGER")
        {
            // Logger receives messages routed to it (or broadcast messages)
            std::cout << "[LOGGER] Monitoring messages..." << std::endl;

            std::string sourceIdentity;
            Frame frame;
            if (dealer.receiveFrom(sourceIdentity, frame, 5000))
            {
                receivedCount++;
                MessageParser parser(frame);
                std::cout << "[LOGGER] Message from " << sourceIdentity 
                          << ": node=0x" << std::hex << parser.srcNode()
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
