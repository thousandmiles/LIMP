#pragma once

#include "zmq_transport_base.hpp"
#include <cstddef>

namespace limp
{

    /**
     * @brief ZeroMQ subscriber transport using SUB socket
     *
     * Implements a subscriber that receives published messages. Can
     * subscribe to specific topics or all messages. Multiple topic
     * subscriptions are supported.
     *
     * Example usage:
     * @code
     * ZMQSubscriber subscriber;
     * subscriber.connect("tcp://127.0.0.1:5556");
     * subscriber.subscribe("topic1");
     * subscriber.subscribe("topic2");
     * auto message = subscriber.receive();
     * @endcode
     */
    class ZMQSubscriber : public ZMQTransport
    {
    public:
        /**
         * @brief Construct a ZeroMQ subscriber
         *
         * @param config Configuration parameters
         */
        explicit ZMQSubscriber(const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Connect to a publisher endpoint
         *
         * Establishes connection to the specified publisher.
         *
         * @param endpoint Publisher address (e.g., "tcp://127.0.0.1:5556")
         * @return true on success, false on failure
         */
        bool connect(const std::string &endpoint);

        /**
         * @brief Subscribe to a topic
         *
         * Adds a topic subscription filter. Messages with matching topic
         * prefix will be received. Call with empty string to receive all
         * messages.
         *
         * @param topic Topic to subscribe to (empty for all messages)
         * @return true on success, false on failure
         */
        bool subscribe(const std::string &topic = "");

        /**
         * @brief Unsubscribe from a topic
         *
         * Removes a topic subscription filter.
         *
         * @param topic Topic to unsubscribe from
         * @return true on success, false on failure
         */
        bool unsubscribe(const std::string &topic);

        /**
         * @brief Not supported for subscriber (receive uses receiveWithTopic)
         * @return false
         */
        bool send(const Frame &frame) override;

        /**
         * @brief Receive a LIMP frame (strips topic prefix automatically)
         *
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds
         * @return true on success, false on timeout or error
         */
        bool receive(Frame &frame, int timeoutMs = -1) override;

        /**
         * @brief Not supported for subscriber
         *
         * Subscribers do not send data.
         *
         * @return Always false
         */
        bool send(const uint8_t *data, size_t size);

        /**
         * @brief Receive a published message
         *
         * Blocks until a message matching subscribed topics is received
         * or timeout occurs. If topic filtering is used, the topic prefix
         * is included in the received data.
         *
         * @param buffer Buffer to store received data
         * @param maxSize Maximum buffer size
         * @return Number of bytes received, 0 on timeout, -1 on error
         */
        std::ptrdiff_t receive(uint8_t *buffer, size_t maxSize);
    };

} // namespace limp
