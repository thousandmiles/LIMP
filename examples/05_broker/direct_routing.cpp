/**
 * @file direct_routing.cpp
 * @brief Example demonstrating direct destination-based routing with send/receive
 *
 * This example shows how to use ZMQDealer::send(destination, frame) and ZMQRouter::receive()
 * with destination identities for explicit peer-to-peer routing through a router.
 *
 * Pattern: DEALER clients use send(destination, frame) to specify recipient.
 * Router receives both source and destination identities, then forwards accordingly.
 *
 * Message flow:
 *   1. Client A: dealer.send("ClientB_Identity", frame)
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
#include <mutex>

using namespace limp;

std::atomic<bool> running{true};
std::mutex coutMutex;  // Mutex for synchronized console output

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
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[Router] Starting direct routing server..." << std::endl;
    }

    ZMQConfig config;
    config.receiveTimeout = 1000;
    config.sendTimeout = 1000;

    ZMQRouter router(config);
    router.setErrorCallback([](const std::string &msg)
                            { 
                                std::lock_guard<std::mutex> lock(coutMutex);
                                std::cerr << "[Router] Error: " << msg << std::endl; 
                            });

    auto bindErr = router.bind("tcp://*:5555");
    if (bindErr != TransportError::None)
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "[Router] Failed to bind: " << toString(bindErr) << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[Router] Listening on tcp://*:5555" << std::endl;
        std::cout << "[Router] Ready to route messages between clients" << std::endl;
    }

    // Track client identities
    std::map<uint16_t, std::string> nodeRegistry; // nodeID -> socket identity

    while (running)
    {
        std::string sourceIdentity;
        std::string destinationIdentity;
        Frame frame;

        // Receive message with source and destination
        auto recvErr = router.receive(sourceIdentity, destinationIdentity, frame, 1000);
        if (recvErr == TransportError::None)
        {
            uint16_t srcNodeId = frame.srcNodeID;

            // Register source node
            if (nodeRegistry.find(srcNodeId) == nodeRegistry.end())
            {
                nodeRegistry[srcNodeId] = sourceIdentity;
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "[Router] Registered " << sourceIdentity << " (node 0x" << std::hex << srcNodeId << std::dec << ")" << std::endl;
            }

            // Forward to the destination identity that the client specified
            if (!destinationIdentity.empty())
            {
                auto sendErr = router.send(destinationIdentity, sourceIdentity, frame);
                if (sendErr == TransportError::None)
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "[Router] Routed from " << sourceIdentity << " to " << destinationIdentity << std::endl;
                }
                else
                {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cerr << "[Router] Failed to route from " << sourceIdentity << " to " << destinationIdentity << std::endl;
                }
            }
            else
            {
                std::lock_guard<std::mutex> lock(coutMutex);
                std::cerr << "[Router] No destination specified from " << sourceIdentity << std::endl;
            }
        }
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[Router] Shutting down..." << std::endl;
    }
}

// Client node using send(dst, frame) for direct routing
void clientThread(const std::string &name, uint16_t nodeId, uint16_t targetNodeId, int delaySeconds)
{
    std::this_thread::sleep_for(std::chrono::seconds(delaySeconds));

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[" << name << "] Starting (node 0x" << std::hex << nodeId << std::dec << ")" << std::endl;
    }

    ZMQConfig config;
    config.receiveTimeout = 1000;
    config.sendTimeout = 1000;

    ZMQDealer dealer(config);
    dealer.setErrorCallback([name](const std::string &msg)
                            { 
                                std::lock_guard<std::mutex> lock(coutMutex);
                                std::cerr << "[" << name << "] Error: " << msg << std::endl; 
                            });

    // Set explicit identity so other clients can address this client
    std::string myIdentity = "CLIENT_" + std::to_string(nodeId);
    auto idErr = dealer.setIdentity(myIdentity);
    if (idErr != TransportError::None)
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "[" << name << "] Failed to set identity: " << toString(idErr) << std::endl;
        return;
    }

    auto connectErr = dealer.connect("tcp://localhost:5555");
    if (connectErr != TransportError::None)
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cerr << "[" << name << "] Failed to connect: " << toString(connectErr) << std::endl;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[" << name << "] Connected with identity: " << myIdentity << std::endl;
    }

    // Send registration message using send(dst, frame) to router ("ROUTER" identity)
    Frame regFrame = MessageBuilder()
                         .setSrcNode(nodeId)
                         .setMsgType(MsgType::EVENT)
                         .setClass(0)
                         .setInstance(0)
                         .setAttribute(0)
                         .setPayload(std::vector<uint8_t>{0x01}) // Registration payload
                         .build();

    // Use "ROUTER" as the destination for registration
    auto sendErr = dealer.send("ROUTER", regFrame);
    if (sendErr == TransportError::None)
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[" << name << "] Sent registration to ROUTER" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    int messageCount = 0;
    int receivedCount = 0;

    while (running && messageCount < 5)
    {
        // Send message to specific destination using send(dst, frame)
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

        // Use the target client's known identity
        // Since we set explicit identities, we can address them directly
        std::string destIdentity = "CLIENT_" + std::to_string(targetNodeId);

        auto sendErr = dealer.send(destIdentity, frame);
        if (sendErr == TransportError::None)
        {
            messageCount++;
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "[" << name << "] Sent #" << messageCount << " to " << destIdentity 
                      << " (target 0x" << std::hex << targetNodeId << std::dec << ")" << std::endl;
        }
        else
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cerr << "[" << name << "] Failed to send" << std::endl;
        }

        // Try to receive responses
        Frame response;
        std::string sourceIdentity;
        auto recvErr = dealer.receive(sourceIdentity, response, 1000);
        if (recvErr == TransportError::None)
        {
            receivedCount++;
            std::string receivedPayload(response.payload.begin(),
                                        response.payload.end());
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "[" << name << "] Received #" << receivedCount
                      << " from " << sourceIdentity << " (node 0x" << std::hex << response.srcNodeID 
                      << std::dec << "): " << receivedPayload << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Wait for remaining responses
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[" << name << "] Waiting for remaining responses..." << std::endl;
    }
    auto timeout = std::chrono::steady_clock::now() + std::chrono::seconds(5);

    while (running && receivedCount < messageCount && std::chrono::steady_clock::now() < timeout)
    {
        Frame response;
        std::string sourceIdentity;
        auto recvErr = dealer.receive(sourceIdentity, response, 1000);
        if (recvErr == TransportError::None)
        {
            receivedCount++;
            std::string receivedPayload(response.payload.begin(),
                                        response.payload.end());
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "[" << name << "] Received #" << receivedCount
                      << " from " << sourceIdentity << " (node 0x" << std::hex << response.srcNodeID 
                      << std::dec << "): " << receivedPayload << std::endl;
        }
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[" << name << "] Stats: sent=" << messageCount
                  << ", received=" << receivedCount << std::endl;
    }
}

int main()
{
    std::cout << "=== LIMP Direct Routing Example (send(dst, frame)/receive) ===" << std::endl;
    std::cout << std::endl;
    std::cout << "This example demonstrates explicit destination routing:" << std::endl;
    std::cout << "  - Clients use send(dst, frame) to specify recipient" << std::endl;
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
