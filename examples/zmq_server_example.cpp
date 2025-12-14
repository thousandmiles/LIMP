/**
 * @file zmq_server_example.cpp
 * @brief Example demonstrating ZeroMQ REQ-REP server pattern
 *
 * This example shows how to use the ZMQServer class to receive requests
 * and send responses using the LIMP protocol over ZeroMQ.
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <csignal>
#include <atomic>

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
    std::cout << "=== LIMP ZeroMQ Server Example ===" << std::endl;
    std::cout << std::endl;

    // Setup signal handler
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    // Configure ZeroMQ transport
    ZMQConfig config;
    config.receiveTimeout = 1000; // 1 second receive timeout (for responsive shutdown)
    config.sendTimeout = 3000;    // 3 second send timeout
    config.lingerTime = 1000;     // 1 second linger on close

    // Create server transport
    ZMQServer server(config);

    // Set error callback
    server.setErrorCallback([](const std::string &errorMsg)
                            { std::cerr << "Error: " << errorMsg << std::endl; });

    // Bind to endpoint
    std::string endpoint = "tcp://*:5555";
    std::cout << "Binding to " << endpoint << "..." << std::endl;

    if (!server.bind(endpoint))
    {
        std::cerr << "Failed to bind to endpoint" << std::endl;
        return 1;
    }

    std::cout << "Server listening on " << endpoint << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    std::cout << std::endl;

    // Main server loop
    int requestCount = 0;

    while (running)
    {
        // Receive request
        uint8_t requestBuffer[1024];
        std::ptrdiff_t received = server.receive(requestBuffer, sizeof(requestBuffer));

        if (received < 0)
        {
            std::cerr << "Receive error" << std::endl;
            continue;
        }
        else if (received == 0)
        {
            // Timeout - check if we should continue
            continue;
        }

        requestCount++;
        std::cout << "--- Request " << requestCount << " ---" << std::endl;
        std::cout << "Received request (" << received << " bytes)" << std::endl;

        // Deserialize request
        Frame requestFrame;
        if (!deserializeFrame(std::vector<uint8_t>(requestBuffer, requestBuffer + received), requestFrame))
        {
            std::cerr << "Failed to deserialize request" << std::endl;

            // Send error response
            auto errorBuilder = MessageBuilder::error(
                0x0030,
                0x3000,
                0,
                0,
                ErrorCode::BadPayload);
            Frame errorFrame = errorBuilder.build();

            std::vector<uint8_t> errorData;
            if (serializeFrame(errorFrame, errorData))
            {
                server.send(errorData.data(), errorData.size());
            }
            continue;
        }

        // Display request information
        std::cout << "Request Type: " << toString(requestFrame.msgType) << std::endl;
        std::cout << "Source Node: 0x" << std::hex << requestFrame.srcNodeID << std::dec << std::endl;
        std::cout << "Class ID: 0x" << std::hex << requestFrame.classID << std::dec << std::endl;
        std::cout << "Instance ID: " << requestFrame.instanceID << std::endl;
        std::cout << "Attribute ID: " << requestFrame.attrID << std::endl;
        std::cout << "Payload Type: " << toString(requestFrame.payloadType) << std::endl;

        // Parse payload if present
        if (requestFrame.payloadLen > 0)
        {
            MessageParser parser(requestFrame);
            auto value = parser.getValue();
            if (std::holds_alternative<uint32_t>(value))
            {
                std::cout << "Payload Value (uint32): " << std::get<uint32_t>(value) << std::endl;
            }
        }

        // Build response
        auto responseBuilder = MessageBuilder::response(
            requestFrame.srcNodeID,
            requestFrame.classID,
            requestFrame.instanceID,
            requestFrame.attrID);

        responseBuilder.setPayload(static_cast<uint32_t>(requestCount));
        Frame responseFrame = responseBuilder.build();

        // Serialize and send response
        std::vector<uint8_t> responseData;
        if (!serializeFrame(responseFrame, responseData))
        {
            std::cerr << "Failed to serialize response" << std::endl;
            continue;
        }

        std::cout << "Sending response (" << responseData.size() << " bytes)..." << std::endl;

        if (!server.send(responseData.data(), responseData.size()))
        {
            std::cerr << "Failed to send response" << std::endl;
            continue;
        }

        std::cout << "Response sent successfully" << std::endl;
        std::cout << std::endl;
    }

    std::cout << std::endl;
    std::cout << "=== Server shutting down ===" << std::endl;
    std::cout << "Total requests processed: " << requestCount << std::endl;

    return 0;
}
