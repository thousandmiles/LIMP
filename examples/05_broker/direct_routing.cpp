/**
 * @file direct_routing.cpp
 * @brief Example demonstrating direct destination-based routing with sendTo/receive
 *
 * This example shows how to use ZMQDealer::sendTo() and ZMQRouter::receive()
 * with destination identities for explicit peer-to-peer routing through a router.
 *
 * Pattern: DEALER clients use sendTo(destination, frame) to specify recipient.
 * Router receives both source and destination identities, then forwards accordingly.
 *
 * Message flow:
 *   1. Client A: dealer.sendTo("ClientB_Identity", frame)
 *      Sends: [dest_identity][delimiter][data]
 *   
 *   2. Router receives: [source_identity][dest_identity][delimiter][data] (4 parts)
 *      Using: router.receive(source, dest, frame)
 *   
 *   3. Router forwards to destination: router.send(dest_identity, frame)
 *
 * Note: In this demo, clients send to the ACTUAL socket identity of the destination.
 *       In real applications, you'd maintain a directory service or discovery mechanism.
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <csignal>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

using namespace limp;

std::atomic<bool> running{true};

// Helper function to convert binary identity to hex string for display
std::string identityToHex(const std::string &identity)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char c : identity)
    {
        oss << std::setw(2) << static_cast<int>(c);
    }
    return oss.str();
}

void signalHandler(int signal)
{
    (void)signal;
    running = false;
}

// Router that forwards messages based on explicit destination
void routerThread()
{
    std::cout << "[Router] Starting direct routing server..." << std::endl;

    ZMQConfig config;
    config.receiveTimeout = 1000;
    config.sendTimeout = 1000;

    ZMQRouter router(config);
    router.setErrorCallback([](const std::string &msg) {
        std::cerr << "[Router] Error: " << msg << std::endl;
    });

    if (!router.bind("tcp://*:5555"))
    {
        std::cerr << "[Router] Failed to bind" << std::endl;
        return;
    }

    std::cout << "[Router] Listening on tcp://*:5555" << std::endl;
    std::cout << "[Router] Ready to route messages between clients" << std::endl;

    // Track client identities
    std::map<uint16_t, std::string> nodeRegistry; // nodeID -> socket identity

    while (running)
    {
        std::string sourceIdentity;
        std::string destinationIdentity;
        Frame frame;

        // Receive message with source and destination
        if (router.receive(sourceIdentity, destinationIdentity, frame, 1000))
        {
            uint16_t srcNodeId = frame.srcNodeID;
            
            // Register source node
            if (nodeRegistry.find(srcNodeId) == nodeRegistry.end())
            {
                nodeRegistry[srcNodeId] = sourceIdentity;
                std::cout << "[Router] Registered node 0x" << std::hex << srcNodeId 
                          << " with identity: 0x" << identityToHex(sourceIdentity) << std::dec << std::endl;
            }

            std::cout << "[Router] Routing message:" << std::endl;
            std::cout << "  From: Node 0x" << std::hex << srcNodeId 
                      << " (identity: 0x" << identityToHex(sourceIdentity) << ")" << std::dec << std::endl;
            std::cout << "  To: (requested dest: 0x" << identityToHex(destinationIdentity) << ")" << std::endl;
            std::cout << "  Type: " << static_cast<int>(frame.msgType) << std::dec << std::endl;

            // The destination identity is provided by the sender in sendTo()
            // We could use it directly or look up in registry
            // For now, forward to the requested destination identity
            if (!destinationIdentity.empty())
            {
                std::cout << "  Forwarding to destination identity: 0x" << identityToHex(destinationIdentity) << std::endl;
                
                if (router.send(destinationIdentity, frame))
                {
                    std::cout << "  ✓ Routed successfully" << std::endl;
                }
                else
                {
                    std::cerr << "  ✗ Failed to forward message" << std::endl;
                }
            }
            else
            {
                std::cerr << "  ✗ No destination identity specified" << std::endl;
            }

            std::cout << std::endl;
        }
    }

    std::cout << "[Router] Shutting down..." << std::endl;
}

// Client node using sendTo for direct routing
void clientThread(const std::string &name, uint16_t nodeId, uint16_t targetNodeId, int delaySeconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));

    std::cout << "[" << name << "] Starting client (node 0x" << std::hex << nodeId 
              << ")" << std::dec << std::endl;

    ZMQConfig config;
    config.receiveTimeout = 1000;
    config.sendTimeout = 1000;

    ZMQDealer dealer(config);
    dealer.setErrorCallback([name](const std::string &msg) {
        std::cerr << "[" << name << "] Error: " << msg << std::endl;
    });

    if (!dealer.connect("tcp://localhost:5555"))
    {
        std::cerr << "[" << name << "] Failed to connect" << std::endl;
        return;
    }

    std::cout << "[" << name << "] Connected to router" << std::endl;

    // Send registration message using sendTo to router ("ROUTER" identity)
    Frame regFrame = MessageBuilder()
        .setSrcNode(nodeId)
        .setMsgType(MsgType::EVENT)
        .setClass(0)
        .setInstance(0)
        .setAttribute(0)
        .setPayload(std::vector<uint8_t>{0x01}) // Registration payload
        .build();

    // Use "ROUTER" as the destination for registration
    if (dealer.sendTo("ROUTER", regFrame))
    {
        std::cout << "[" << name << "] Sent registration to ROUTER" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int messageCount = 0;
    int receivedCount = 0;

    while (running && messageCount < 5)
    {
        // Send message to specific destination using sendTo
        std::string payload = name + " message #" + std::to_string(messageCount + 1);
        std::vector<uint8_t> payloadBytes(payload.begin(), payload.end());

        Frame frame = MessageBuilder()
            .setSrcNode(nodeId)
            .setMsgType(MsgType::REQUEST)
            .setClass(1)
            .setInstance(targetNodeId) // Use instance as target for demo
            .setAttribute(1)
            .setPayload(payloadBytes)
            .build();

        // Convert targetNodeId to string for destination identity
        // Note: In practice, you'd use actual socket identities
        std::string destIdentity = "Node_" + std::to_string(targetNodeId);

        std::cout << "[" << name << "] Sending to destination: " << destIdentity << std::endl;
        
        if (dealer.sendTo(destIdentity, frame))
        {
            messageCount++;
            std::cout << "[" << name << "] Sent message #" << messageCount 
                      << " to node 0x" << std::hex << targetNodeId << std::dec << std::endl;
        }
        else
        {
            std::cerr << "[" << name << "] Failed to send message" << std::endl;
        }

        // Try to receive responses
        Frame response;
        if (dealer.receive(response, 1000))
        {
            receivedCount++;
            std::string receivedPayload(response.payload.begin(), 
                                       response.payload.end());
            std::cout << "[" << name << "] ← Received response #" << receivedCount 
                      << " from node 0x" << std::hex << response.srcNodeID 
                      << std::dec << ": " << receivedPayload << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Wait for remaining responses
    std::cout << "[" << name << "] Sent all messages, waiting for remaining responses..." << std::endl;
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    
    while (running && receivedCount < messageCount && std::chrono::steady_clock::now() < timeout)
    {
        Frame response;
        if (dealer.receive(response, 1000))
        {
            receivedCount++;
            std::string receivedPayload(response.payload.begin(), 
                                       response.payload.end());
            std::cout << "[" << name << "] ← Received response #" << receivedCount 
                      << " from node 0x" << std::hex << response.srcNodeID 
                      << std::dec << ": " << receivedPayload << std::endl;
        }
    }

    std::cout << "[" << name << "] Final stats: sent=" << messageCount 
              << ", received=" << receivedCount << std::endl;
    std::cout << "[" << name << "] Shutting down..." << std::endl;
}

int main()
{
    std::cout << "=== LIMP Direct Routing Example (sendTo/receive) ===" << std::endl;
    std::cout << std::endl;
    std::cout << "This example demonstrates explicit destination routing:" << std::endl;
    std::cout << "  - Clients use sendTo(destination, frame) to specify recipient" << std::endl;
    std::cout << "  - Router extracts both source and destination identities" << std::endl;
    std::cout << "  - Router forwards messages to the specified destination" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: This demo shows the routing mechanism. Clients send to" << std::endl;
    std::cout << "      fictional identities to demonstrate the pattern. In a real" << std::endl;
    std::cout << "      application, you'd implement identity discovery/registration." << std::endl;
    std::cout << std::endl;

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Start router thread
    std::thread router(routerThread);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Start client threads
    // Client A (0x1001) sends to Client B (0x1002)
    // Client B (0x1002) sends to Client A (0x1001)
    std::thread clientA(clientThread, "Client-A", 0x1001, 0x1002, 1);
    std::thread clientB(clientThread, "Client-B", 0x1002, 0x1001, 2);

    std::cout << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Wait for threads
    clientA.join();
    clientB.join();
    
    running = false;
    router.join();

    std::cout << std::endl;
    std::cout << "Example completed" << std::endl;

    return 0;
}
