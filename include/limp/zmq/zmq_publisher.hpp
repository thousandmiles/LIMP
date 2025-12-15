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
         * @return true on success, false on failure
         */
        bool bind(const std::string &endpoint);

        /**
         * @brief Publish a LIMP frame with topic
         *
         * Sends a Frame with a topic prefix. Subscribers can filter
         * messages by subscribing to specific topics.
         *
         * @param topic Topic string for filtering
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool publish(const std::string &topic, const Frame &frame);

        /**
         * @brief Send a LIMP frame
         *
         * Serializes and sends a LIMP frame to all subscribers.
         *
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool send(const Frame &frame) override;

        /**
         * @brief Not supported for publisher
         * @return false
         */
        bool receive(Frame &frame, int timeoutMs = -1) override;

    private:
        /**
         * @brief Internal helper to send raw data
         */
        bool send(const uint8_t *data, size_t size);

        /**
         * @brief Internal helper to publish raw data with topic
         */
        bool publish(const std::string &topic, const uint8_t *data, size_t size);
    };

} // namespace limp
