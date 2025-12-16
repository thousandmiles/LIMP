/**
 * @file broker_node.cpp
 * @brief Example showing multiple nodes communicating through a central broker
 *
 * This example demonstrates node-to-node communication through a ROUTER-ROUTER
 * proxy broker. Messages are routed by destination identity.
 *
 * Message Flow:
 *   HMI-001 --[sendTo("PLC-001", msg)]--> Broker --[forward]--> PLC-001
 *   PLC-001 --[sendTo("HMI-001", reply)]--> Broker --[forward]--> HMI-001
 *
 * Run this alongside router_broker.cpp (NOT proxy_broker):
 * 1. Terminal 1: ./router_broker (starts the message broker)
 * 2. Terminal 2: NODE_TYPE=PLC ./broker_node (PLC node)
 * 3. Terminal 3: NODE_TYPE=HMI ./broker_node (HMI node)
 * 4. Terminal 4: NODE_TYPE=LOGGER ./broker_node (Logger node)
 *
 * Note: router_broker.cpp uses custom routing logic to deliver messages
 * based on destination. proxy_broker.cpp does NOT support this pattern.
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
    // For ROUTER-ROUTER proxy message broker: all nodes connect to frontend
    // The proxy doesn't route by destination in messages - it routes by socket
    // For true message broker with destination routing, use router_broker.cpp instead
    std::string brokerEndpoint = "tcp://127.0.0.1:5555"; // All connect to frontend

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

    // Send initial registration message so broker knows about this node
    // This is important for nodes that wait to receive (like PLC)
    std::cout << "[" << nodeType << "] Registering with broker..." << std::endl;
    Frame regMsg = MessageBuilder::event(nodeID, 0x0001, 0x0001, 0x0001).build();
    dealer.send(regMsg);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int sentCount = 0;
    int receivedCount = 0;

    // Main communication loop
    while (running)
    {
        // Different behavior based on node type
        if (nodeType == "HMI")
        {
            // HMI sends commands (router_broker echoes back responses)
            sentCount++;
            std::cout << "[HMI] Sending REQUEST #" << sentCount << "..." << std::endl;

            Frame cmd = MessageBuilder::request(nodeID, 0x2000, 0x0001, 0x0001).build();

            // Use regular send (router_broker handles routing)
            dealer.send(cmd);

            // Try to receive response
            Frame response;
            if (dealer.receive(response, 3000))
            {
                receivedCount++;
                MessageParser parser(response);
                std::cout << "[HMI] Received RESPONSE #" << receivedCount
                          << " (from node: 0x" << std::hex << parser.srcNode() << std::dec << ")" << std::endl;
            }
            else
            {
                std::cout << "[HMI] No response received (timeout)" << std::endl;
            }

            // Also send an EVENT periodically (every other loop)
            if (sentCount % 2 == 0)
            {
                std::cout << "[HMI] Sending EVENT (user action logged)..." << std::endl;
                Frame event = MessageBuilder::event(nodeID, 0x1000, 0x0001, 0x0001).build();
                dealer.send(event);
            }

            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        else if (nodeType == "PLC")
        {
            // PLC waits for commands and responds (router_broker handles this)
            std::cout << "[PLC] Waiting for requests..." << std::endl;

            Frame request;
            if (dealer.receive(request, 5000))
            {
                receivedCount++;
                MessageParser parser(request);
                std::cout << "[PLC] Received REQUEST from node 0x" << std::hex
                          << parser.srcNode() << std::dec << std::endl;

                // Send response (router_broker routes back to sender)
                sentCount++;
                Frame response = MessageBuilder::response(nodeID, parser.classID(),
                                                          parser.instanceID(),
                                                          parser.attrID())
                                     .build();

                dealer.send(response);
                std::cout << "[PLC] RESPONSE sent" << std::endl;

                // Send EVENT to notify about the processing
                std::cout << "[PLC] Sending EVENT (request processed)..." << std::endl;
                Frame event = MessageBuilder::event(nodeID, 0x3000, 0x0001, 0x0001).build();
                dealer.send(event);
                sentCount++;
            }
        }
        else if (nodeType == "LOGGER")
        {
            // Logger receives broadcast events
            std::cout << "[LOGGER] Monitoring for events..." << std::endl;

            Frame frame;
            if (dealer.receive(frame, 5000))
            {
                receivedCount++;
                MessageParser parser(frame);
                std::cout << "[LOGGER] Event from node 0x" << std::hex << parser.srcNode()
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
