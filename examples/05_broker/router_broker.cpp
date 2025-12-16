/**
 * @file router_broker.cpp
 * @brief Message broker using ZMQRouter for custom routing logic
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

    std::cout << "Broker Mode: Routes messages based on message type" << std::endl;
    std::cout << "  - REQUEST  → PLC nodes (0x0030)" << std::endl;
    std::cout << "  - RESPONSE → HMI nodes (0x0010)" << std::endl;
    std::cout << "  - EVENT    → All registered nodes (broadcast)" << std::endl;
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

        // BROKER LOGIC: Route messages between nodes
        
        // Strategy 1: Route REQUEST messages to PLC, get response, send back to requester
        if (parser.msgType() == MsgType::REQUEST)
        {
            std::cout << "  [ROUTING] Request to PLC nodes" << std::endl;
            
            // Find PLC nodes and forward the request
            bool routed = false;
            for (const auto &[nodeId, destIdentity] : routingTable)
            {
                // Route to PLC nodes (0x0030 is PLC node ID)
                if (nodeId == 0x0030)
                {
                    router.send(destIdentity, incomingFrame);
                    std::cout << "    -> Forwarded to PLC: " << destIdentity << std::endl;
                    routed = true;
                }
            }
            
            if (!routed)
            {
                std::cout << "    [WARNING] No PLC nodes registered" << std::endl;
            }
        }
        
        // Strategy 2: Route RESPONSE messages back to original requester
        else if (parser.msgType() == MsgType::RESPONSE)
        {
            std::cout << "  [ROUTING] Response to requesters" << std::endl;
            
            // Find HMI/requester nodes and forward the response
            for (const auto &[nodeId, destIdentity] : routingTable)
            {
                // Route to HMI nodes (0x0010 is HMI node ID)
                if (nodeId == 0x0010 && destIdentity != sourceIdentity)
                {
                    router.send(destIdentity, incomingFrame);
                    std::cout << "    -> Forwarded to HMI: " << destIdentity << std::endl;
                }
            }
        }
        
        // Strategy 3: Broadcast EVENT messages to all registered nodes (except sender)
        else if (parser.msgType() == MsgType::EVENT)
        {
            std::cout << "  [BROADCAST] Event to all nodes" << std::endl;
            int broadcastCount = 0;
            for (const auto &[nodeId, destIdentity] : routingTable)
            {
                if (destIdentity != sourceIdentity)  // Don't echo to sender
                {
                    router.send(destIdentity, incomingFrame);
                    std::cout << "    -> Node 0x" << std::hex << nodeId 
                              << std::dec << " (" << destIdentity << ")" << std::endl;
                    broadcastCount++;
                }
            }
            if (broadcastCount == 0)
            {
                std::cout << "    [INFO] No other nodes to broadcast to" << std::endl;
            }
        }

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
