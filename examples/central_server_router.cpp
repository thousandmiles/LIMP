/**
 * @file central_server_router.cpp
 * @brief Central server using ZMQRouter as a message broker
 *
 * This implementation shows how to build a working broker with ZMQRouter by:
 * - Maintaining a routing table (srcNode → socket identity)
 * - Auto-registering clients on first message
 * - Routing messages using different strategies:
 *   1. Echo/Response mode (REQUEST → RESPONSE)
 *   2. Broadcast mode (EVENT → all nodes)
 *   3. Direct routing (can use classID or custom field as destination)
 *
 * ROUTING TABLE APPROACH:
 * Instead of parsing destination from multipart frames, this broker:
 * - Learns node identities from srcNode in received frames
 * - Maps srcNode ID → socket identity
 * - Routes based on Frame's internal fields (classID, instanceID, etc.)
 *
 * ADVANTAGES:
 * - Full message inspection and processing
 * - Custom routing logic (broadcast, echo, targeted)
 * - Application-layer validation/filtering
 * - Detailed logging and monitoring
 *
 * LIMITATION:
 * - Requires consistent srcNode IDs from clients
 * - Cannot use dealer.sendTo() pattern (use regular send())
 * - More complex than ZMQProxy for simple forwarding
 *
 * Use when you need:
 * - Message inspection/processing
 * - Custom routing strategies
 * - Broadcasting capabilities
 * - Application-layer security
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <unordered_map>

using namespace limp;

std::atomic<bool> running{true};

void signalHandler(int signal)
{
    (void)signal;
    running = false;
}

int main()
{
    std::cout << "=== Central Server (Router Approach) ===" << std::endl;
    std::cout << "This server manually handles each message with custom logic" << std::endl;
    std::cout << std::endl;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Configure ZeroMQ
    ZMQConfig config;
    config.receiveTimeout = 1000;
    config.sendTimeout = 3000;

    // Create router server
    ZMQRouter router(config);

    router.setErrorCallback([](const std::string &errorMsg)
                            { std::cerr << "[SERVER] Error: " << errorMsg << std::endl; });

    std::string endpoint = "tcp://0.0.0.0:5555";
    std::cout << "Starting router server on " << endpoint << std::endl;

    if (!router.bind(endpoint))
    {
        std::cerr << "Failed to bind to " << endpoint << std::endl;
        return 1;
    }

    std::cout << "Server running. Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Track statistics
    std::unordered_map<std::string, int> nodeMessageCount;
    int totalMessages = 0;
    
    // Routing table: maps srcNode ID -> socket identity
    // This is built dynamically as clients connect
    std::unordered_map<uint16_t, std::string> routingTable;

    std::cout << "Broker Mode: Routes messages based on Frame destination node" << std::endl;
    std::cout << "Clients should use regular dealer.send() (not sendTo)" << std::endl;
    std::cout << std::endl;

    // Main server loop - broker messages between nodes
    while (running)
    {
        std::string sourceIdentity;
        Frame incomingFrame;

        // Receive message from any client
        if (!router.receive(sourceIdentity, incomingFrame, 1000))
        {
            continue; // Timeout, check if still running
        }

        totalMessages++;
        nodeMessageCount[sourceIdentity]++;

        // Parse the incoming message
        MessageParser parser(incomingFrame);

        std::cout << "[RECEIVED] From: " << sourceIdentity
                  << " | SrcNode: 0x" << std::hex << parser.srcNode()
                  << " | Type: 0x" << static_cast<int>(parser.msgType())
                  << std::dec << std::endl;

        // Register this node in routing table (first message from this node)
        if (routingTable.find(parser.srcNode()) == routingTable.end())
        {
            routingTable[parser.srcNode()] = sourceIdentity;
            std::cout << "  [REGISTERED] Node 0x" << std::hex << parser.srcNode() 
                      << std::dec << " -> " << sourceIdentity << std::endl;
        }

        // BROKER LOGIC: Route to destination based on Frame's internal addressing
        // For LIMP, we can use the classID or instanceID as destination indicator
        // Or extend the Frame to include a destination node field
        
        // Example routing strategies:
        
        // Strategy 1: Echo back to sender (testing/response mode)
        if (parser.msgType() == MsgType::REQUEST)
        {
            Frame response = MessageBuilder::response(
                                 0x0100, // Broker node ID
                                 parser.classID(),
                                 parser.instanceID(),
                                 parser.attrID())
                                 .build();

            router.send(sourceIdentity, response);
            std::cout << "  [SENT] Response to: " << sourceIdentity << std::endl;
        }
        
        // Strategy 2: Broadcast to all registered nodes (except sender)
        else if (parser.msgType() == MsgType::EVENT)
        {
            std::cout << "  [BROADCAST] Event to all nodes" << std::endl;
            for (const auto &[nodeId, destIdentity] : routingTable)
            {
                if (destIdentity != sourceIdentity)  // Don't echo to sender
                {
                    router.send(destIdentity, incomingFrame);
                    std::cout << "    -> " << destIdentity << std::endl;
                }
            }
        }
        
        // Strategy 3: Route to specific node
        // For this to work, you'd need to extend Frame protocol to include
        // a destination node field, or use classID as destination indicator
        // Example: if classID represents target node ID
        /*
        else
        {
            uint16_t destNodeId = parser.classID();  // Treat classID as destination
            auto it = routingTable.find(destNodeId);
            if (it != routingTable.end())
            {
                router.send(it->second, incomingFrame);
                std::cout << "  [ROUTED] To node 0x" << std::hex << destNodeId 
                          << std::dec << " (" << it->second << ")" << std::endl;
            }
            else
            {
                std::cout << "  [ERROR] Destination node 0x" << std::hex 
                          << destNodeId << std::dec << " not found" << std::endl;
            }
        }
        */

        std::cout << "  Total messages: " << totalMessages 
                  << " | Registered nodes: " << routingTable.size() << std::endl;
        std::cout << std::endl;
    }

    std::cout << "\nShutting down server..." << std::endl;
    std::cout << "Final statistics:" << std::endl;
    std::cout << "  Total messages: " << totalMessages << std::endl;
    std::cout << "  Clients:" << std::endl;
    for (const auto &[identity, count] : nodeMessageCount)
    {
        std::cout << "    " << identity << ": " << count << " messages" << std::endl;
    }

    router.close();

    return 0;
}
