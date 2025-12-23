/**
 * @file dealer.cpp
 * @brief Example demonstrating ZeroMQ DEALER pattern for asynchronous clients
 *
 * This example shows how to use the ZMQDealer class to send asynchronous
 * requests to a ROUTER server without strict send-receive alternation.
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include <iomanip>

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
    std::cout << "=== LIMP ZeroMQ Dealer Example ===" << std::endl;
    std::cout << std::endl;

    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Configure ZeroMQ transport
    ZMQConfig config;
    config.receiveTimeout = 5000; // 5 second receive timeout
    config.sendTimeout = 3000;    // 3 second send timeout
    config.lingerTime = 1000;     // 1 second linger on close

    // Create dealer transport
    ZMQDealer dealer(config);

    // Set error callback (filter out timeout errors)
    dealer.setErrorCallback([](const std::string &errorMsg)
                            { 
                                // Don't log timeout errors
                                if (errorMsg.find("Resource temporarily unavailable") == std::string::npos) {
                                    std::cerr << "Dealer Error: " << errorMsg << std::endl;
                                }
                            });

    // Set custom identity (optional - ZeroMQ will generate one if not set)
    std::string identity = "DEALER-HMI-001";
    auto idErr = dealer.setIdentity(identity);
    if (idErr != TransportError::None)
    {
        std::cerr << "Warning: Could not set identity: " << toString(idErr) << std::endl;
    }
    else
    {
        std::cout << "Dealer identity set to: " << identity << std::endl;
    }

    // Connect to router
    std::string endpoint = "tcp://127.0.0.1:5555";
    std::cout << "Connecting to router at " << endpoint << "..." << std::endl;

    auto connectErr = dealer.connect(endpoint);
    if (connectErr != TransportError::None)
    {
        std::cerr << "Failed to connect: " << toString(connectErr) << std::endl;
        return 1;
    }

    std::cout << "Connected to router" << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Give router time to register connection
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int messageCount = 0;
    int responseCount = 0;

    // Main loop - send requests periodically
    while (running)
    {
        messageCount++;

        std::cout << "----------------------------------------" << std::endl;
        std::cout << "Sending request #" << messageCount << std::endl;

        // Create a request message
        Frame requestFrame = MessageBuilder::request(0x0010, 0x3000, 0x0001, 0x0001)
                                 .build();

        // Send the request (no need to wait for response immediately)
        auto sendErr = dealer.send(requestFrame);
        if (sendErr != TransportError::None)
        {
            std::cerr << "Failed to send request: " << toString(sendErr) << std::endl;
            continue;
        }

        std::cout << "Request sent to router" << std::endl;

        // Try to receive response (asynchronous - may timeout)
        Frame responseFrame;
        auto recvErr = dealer.receive(responseFrame, 1000);
        if (recvErr == TransportError::None)
        {
            responseCount++;

            // Parse response
            MessageParser parser(responseFrame);
            std::cout << "Received response #" << responseCount << std::endl;
            std::cout << "  Source Node: 0x" << std::hex << std::setw(4) << std::setfill('0')
                      << parser.srcNode() << std::dec << std::endl;
            std::cout << "  Message Type: 0x" << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(parser.msgType()) << std::dec << std::endl;
            std::cout << "  Payload size: " << parser.frame().payloadLen << " bytes" << std::endl;
        }
        else
        {
            std::cout << "No response received (timeout or error)" << std::endl;
        }

        std::cout << "Statistics: Sent=" << messageCount << " Received=" << responseCount << std::endl;

        // Wait before sending next request
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    std::cout << std::endl;
    std::cout << "Shutting down dealer..." << std::endl;
    dealer.close();

    std::cout << std::endl;
    std::cout << "Final Statistics:" << std::endl;
    std::cout << "  Total requests sent: " << messageCount << std::endl;
    std::cout << "  Total responses received: " << responseCount << std::endl;

    return 0;
}
