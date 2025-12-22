/**
 * @file client.cpp
 * @brief Example demonstrating ZeroMQ REQ-REP client pattern
 *
 * This example shows how to use the ZMQClient class to send requests
 * and receive responses using the LIMP protocol over ZeroMQ.
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace limp;

int main()
{
    std::cout << "=== LIMP ZeroMQ Client Example ===" << std::endl;
    std::cout << std::endl;

    // Configure ZeroMQ transport
    ZMQConfig config;
    config.sendTimeout = 3000;    // 3 second send timeout
    config.receiveTimeout = 3000; // 3 second receive timeout
    config.lingerTime = 1000;     // 1 second linger on close

    // Create client transport
    ZMQClient client(config);

    // Set error callback
    client.setErrorCallback([](const std::string &errorMsg)
                            { std::cerr << "Error: " << errorMsg << std::endl; });

    // Connect to server
    std::string endpoint = "tcp://127.0.0.1:5555";
    std::cout << "Connecting to " << endpoint << "..." << std::endl;

    auto connectErr = client.connect(endpoint);
    if (connectErr != TransportError::None)
    {
        std::cerr << "Failed to connect: " << toString(connectErr) << std::endl;
        return 1;
    }

    std::cout << "Connected successfully!" << std::endl;
    std::cout << std::endl;

    // Send multiple requests
    for (int i = 1; i <= 5; ++i)
    {
        std::cout << "--- Request " << i << " ---" << std::endl;

        // Build request message using the actual API
        auto builder = MessageBuilder::request(
            0x0010,
            0x3000,
            100, // Instance ID
            i    // Attribute ID
        );

        builder.setPayload(static_cast<uint32_t>(i * 100));
        Frame requestFrame = builder.build();

        std::cout << "Sending request (" << requestFrame.totalSize() << " bytes)..." << std::endl;

        auto sendErr = client.send(requestFrame);
        if (sendErr != TransportError::None)
        {
            std::cerr << "Failed to send request: " << toString(sendErr) << std::endl;
            continue;
        }

        // Receive response
        Frame responseFrame;
        std::cout << "Waiting for response..." << std::endl;

        auto recvErr = client.receive(responseFrame);
        if (recvErr != TransportError::None)
        {
            std::cerr << "Failed to receive response: " << toString(recvErr) << std::endl;
            continue;
        }

        std::cout << "Received response (" << responseFrame.totalSize() << " bytes)" << std::endl;

        // Display response information
        MessageParser parser(responseFrame);
        std::cout << "Response Type: " << toString(responseFrame.msgType) << std::endl;
        std::cout << "Source Node: 0x" << std::hex << responseFrame.srcNodeID << std::dec << std::endl;
        std::cout << "Class ID: 0x" << std::hex << responseFrame.classID << std::dec << std::endl;
        std::cout << "Instance ID: " << responseFrame.instanceID << std::endl;
        std::cout << "Attribute ID: " << responseFrame.attrID << std::endl;
        std::cout << "Payload Type: " << toString(responseFrame.payloadType) << std::endl;
        std::cout << "Payload Length: " << responseFrame.payloadLen << std::endl;

        // Try to get payload value
        auto value = parser.getValue();
        if (std::holds_alternative<std::string>(value))
        {
            std::cout << "Payload Value (string): " << std::get<std::string>(value) << std::endl;
        }
        else if (std::holds_alternative<uint32_t>(value))
        {
            std::cout << "Payload Value (uint32): " << std::get<uint32_t>(value) << std::endl;
        }

        std::cout << std::endl;

        // Wait before next request
        if (i < 5)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    std::cout << "=== Client finished ===" << std::endl;

    return 0;
}
