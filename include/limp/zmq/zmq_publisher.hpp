#pragma once

#include "zmq_transport_base.hpp"
#include <cstddef>

namespace limp
{

    /**
     * @brief ZeroMQ publisher transport using PUB socket
     *
     * Implements a publisher for one-way message distribution. Sends
     * messages to multiple subscribers without receiving responses.
     * Messages may include topic prefixes for filtering.
     *
     * Note: Subscribers may miss initial messages due to "slow joiner"
     * syndrome. Use warmup period or other synchronization if needed.
     *
     * Example usage:
     * @code
     * ZMQPublisher publisher;
     * publisher.bind("tcp://0.0.0.0:5556");
     * publisher.publish("topic1", eventData);
     * @endcode
     */
    class ZMQPublisher : public ZMQTransport
    {
    public:
        /**
         * @brief Construct a ZeroMQ publisher
         *
         * @param config Configuration parameters
         */
        explicit ZMQPublisher(const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Bind to an endpoint
         *
         * Binds the publisher socket to the specified endpoint.
         *
         * @param endpoint Bind address (e.g., "tcp://0.0.0.0:5556")
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError bind(const std::string &endpoint);

        /**
         * @brief Publish a LIMP frame with topic
         *
         * Publishes a Frame with a topic prefix. Subscribers can filter
         * messages by subscribing to specific topics. Use empty string "" for no topic filtering.
         *
         * @param topic Topic string for filtering (use "" for broadcast to all)
         * @param frame Frame to publish
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError publish(const std::string &topic, const Frame &frame);

        /**
         * @brief Publish raw data with topic
         *
         * @param topic Topic string for filtering (use "" for broadcast to all)
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError publishRaw(const std::string &topic, const uint8_t *data, size_t size);

    private:
        // Base class overrides - use publish() instead
        TransportError send(const Frame &frame) override;
        TransportError sendRaw(const uint8_t *data, size_t size) override;
        TransportError receive(Frame &frame, int timeoutMs = -1) override;
        std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize) override;
    };

} // namespace limp
