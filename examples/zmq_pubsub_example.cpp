/**
 * @file zmq_pubsub_example.cpp
 * @brief Example demonstrating ZeroMQ PUB-SUB pattern
 *
 * This example demonstrates both publisher and subscriber in a single
 * program using threads. In real applications, these would typically
 * run as separate processes.
 */

#include "limp/limp.hpp"
#include "limp/zmq/zmq.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <string>

using namespace limp;

std::atomic<bool> running{true};

/**
 * @brief Publisher thread function
 *
 * Publishes sensor data events to different topics
 */
void publisherThread()
{
    std::cout << "[Publisher] Starting..." << std::endl;

    // Configure and create publisher
    ZMQConfig config;
    config.sendTimeout = 1000;

    ZMQPublisher publisher(config);

    // Bind to endpoint
    if (!publisher.bind("tcp://*:5556"))
    {
        std::cerr << "[Publisher] Failed to bind" << std::endl;
        return;
    }

    std::cout << "[Publisher] Bound to tcp://*:5556" << std::endl;

    // Wait for subscribers to connect (slow joiner problem workaround)
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Publish events
    int eventCount = 0;

    while (running && eventCount < 20)
    {
        eventCount++;

        // Alternate between temperature and pressure topics
        if (eventCount % 2 == 0)
        {
            // Temperature event
            auto tempBuilder = MessageBuilder::event(
                0x0010,
                0x3000,
                1, // Instance ID for temperature
                1  // Attribute ID
            );
            tempBuilder.setPayload(23.5f + (eventCount % 10));
            Frame tempFrame = tempBuilder.build();

            std::vector<uint8_t> tempData;
            if (serializeFrame(tempFrame, tempData))
            {
                publisher.publish("temperature", tempData.data(), tempData.size());
                std::cout << "[Publisher] Published temperature event #" << eventCount << std::endl;
            }
        }
        else
        {
            // Pressure event
            auto pressureBuilder = MessageBuilder::event(
                0x0010,
                0x3000,
                2, // Instance ID for pressure
                1  // Attribute ID
            );
            pressureBuilder.setPayload(101.3f + (eventCount % 5));
            Frame pressureFrame = pressureBuilder.build();

            std::vector<uint8_t> pressureData;
            if (serializeFrame(pressureFrame, pressureData))
            {
                publisher.publish("pressure", pressureData.data(), pressureData.size());
                std::cout << "[Publisher] Published pressure event #" << eventCount << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    std::cout << "[Publisher] Finished" << std::endl;
}

/**
 * @brief Subscriber thread function
 *
 * Subscribes to sensor data topics and processes events
 */
void subscriberThread(const std::string &topic)
{
    std::cout << "[Subscriber " << topic << "] Starting..." << std::endl;

    // Wait for publisher to start
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Configure and create subscriber
    ZMQConfig config;
    config.receiveTimeout = 1000;

    ZMQSubscriber subscriber(config);

    // Connect to publisher
    if (!subscriber.connect("tcp://127.0.0.1:5556"))
    {
        std::cerr << "[Subscriber " << topic << "] Failed to connect" << std::endl;
        return;
    }

    std::cout << "[Subscriber " << topic << "] Connected to tcp://127.0.0.1:5556" << std::endl;

    // Subscribe to topic
    if (!subscriber.subscribe(topic))
    {
        std::cerr << "[Subscriber " << topic << "] Failed to subscribe" << std::endl;
        return;
    }

    std::cout << "[Subscriber " << topic << "] Subscribed to topic: " << topic << std::endl;

    // Receive events
    int eventCount = 0;

    while (running && eventCount < 10)
    {
        uint8_t buffer[1024];
        std::ptrdiff_t received = subscriber.receive(buffer, sizeof(buffer));

        if (received < 0)
        {
            std::cerr << "[Subscriber " << topic << "] Receive error" << std::endl;
            continue;
        }
        else if (received == 0)
        {
            // Timeout - continue
            continue;
        }

        eventCount++;

        // The received data includes the topic prefix
        // Find where the LIMP frame starts (after the topic)
        size_t topicLen = topic.length();
        if (static_cast<size_t>(received) > topicLen)
        {
            // Deserialize LIMP message (skip topic prefix)
            std::vector<uint8_t> frameData(buffer + topicLen, buffer + received);
            Frame eventFrame;

            if (deserializeFrame(frameData, eventFrame))
            {
                MessageParser parser(eventFrame);
                auto value = parser.getValue();

                if (std::holds_alternative<float>(value))
                {
                    float floatValue = std::get<float>(value);
                    std::cout << "[Subscriber " << topic << "] Received event #" << eventCount
                              << ", value: " << floatValue << std::endl;
                }
            }
        }
    }

    std::cout << "[Subscriber " << topic << "] Finished (received " << eventCount << " events)" << std::endl;
}

int main()
{
    std::cout << "=== LIMP ZeroMQ PUB-SUB Example ===" << std::endl;
    std::cout << std::endl;

    // Start publisher thread
    std::thread pubThread(publisherThread);

    // Start subscriber threads for different topics
    std::thread tempSubThread(subscriberThread, "temperature");
    std::thread pressureSubThread(subscriberThread, "pressure");

    // Wait for all threads to complete
    pubThread.join();
    running = false;
    tempSubThread.join();
    pressureSubThread.join();

    std::cout << std::endl;
    std::cout << "=== Example finished ===" << std::endl;

    return 0;
}
