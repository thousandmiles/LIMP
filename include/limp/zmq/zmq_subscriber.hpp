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
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError connect(const std::string &endpoint);

        /**
         * @brief Subscribe to a topic
         *
         * Adds a topic subscription filter. Messages with matching topic
         * prefix will be received. Call with empty string to receive all
         * messages.
         *
         * @param topic Topic to subscribe to (empty for all messages)
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError subscribe(const std::string &topic = "");

        /**
         * @brief Unsubscribe from a topic
         *
         * Removes a topic subscription filter.
         *
         * @param topic Topic to unsubscribe from
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError unsubscribe(const std::string &topic);

        /**
         * @brief Not supported for subscriber (send-only operation)
         * @return TransportError::InternalError
         */
        TransportError send(const Frame &frame) override;

        /**
         * @brief Receive a LIMP frame (strips topic prefix automatically)
         *
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds
         * @return TransportError::None on success, TransportError::Timeout on timeout,
         *         other error code on failure
         */
        TransportError receive(Frame &frame, int timeoutMs = -1) override;

        /**
         * @brief Receive raw data
         * @param buffer Pointer to buffer to store received data
         * @param maxSize Maximum size of the buffer
         */
        std::ptrdiff_t receive(uint8_t *buffer, size_t maxSize);
    };

} // namespace limp
