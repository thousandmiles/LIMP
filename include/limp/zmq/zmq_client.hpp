#pragma once

#include "zmq_transport_base.hpp"
#include <cstddef>

namespace limp
{

    /**
     * @brief ZeroMQ client transport using REQ socket
     *
     * Implements a synchronous request-reply client. Sends requests and
     * waits for responses. Follows ZeroMQ REQ socket semantics:
     * - Must send before receiving
     * - Each send must be followed by a receive
     * - Maintains strict send-receive alternation
     *
     * Example usage:
     * @code
     * ZMQClient client;
     * client.connect("tcp://127.0.0.1:5555");
     * client.send(requestData);
     * auto response = client.receive();
     * @endcode
     */
    class ZMQClient : public ZMQTransport
    {
    public:
        /**
         * @brief Construct a ZeroMQ client
         *
         * @param config Configuration parameters
         */
        explicit ZMQClient(const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Connect to a server endpoint
         *
         * Establishes connection to the specified server endpoint.
         *
         * @param endpoint Server address (e.g., "tcp://127.0.0.1:5555")
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError connect(const std::string &endpoint);

        /**
         * @brief Send a LIMP frame
         *
         * Serializes and sends a LIMP frame via REQ socket.
         *
         * @param frame Frame to send
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError send(const Frame &frame) override;

        /**
         * @brief Receive a LIMP frame
         *
         * Receives and deserializes a LIMP frame.
         *
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds
         * @return TransportError::None on success, TransportError::Timeout on timeout,
         *         other error code on failure
         */
        TransportError receive(Frame &frame, int timeoutMs = -1) override;

        /**
         * @brief Send raw data
         *
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return TransportError::None on success, specific error code on failure
         */
        TransportError send(const uint8_t *data, size_t size);

        /**
         * @brief Receive raw data
         *
         * @param buffer Pointer to buffer to store received data
         * @param maxSize Maximum size of the buffer
         * @return Number of bytes received, or -1 on error
         */
        std::ptrdiff_t receive(uint8_t *buffer, size_t maxSize);
    };

} // namespace limp
