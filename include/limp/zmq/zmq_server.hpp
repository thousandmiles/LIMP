#pragma once

#include "zmq_transport_base.hpp"
#include <cstddef>

namespace limp
{

    /**
     * @brief ZeroMQ server transport using REP socket
     *
     * Implements a synchronous request-reply server. Receives requests
     * and sends responses. Follows ZeroMQ REP socket semantics:
     * - Must receive before sending
     * - Each receive must be followed by a send
     * - Maintains strict receive-send alternation
     *
     * Example usage:
     * @code
     * ZMQServer server;
     * server.bind("tcp://0.0.0.0:5555");
     * while (true) {
     *     auto request = server.receive();
     *     // Process request...
     *     server.send(responseData);
     * }
     * @endcode
     */
    class ZMQServer : public ZMQTransport
    {
    public:
        /**
         * @brief Construct a ZeroMQ server
         *
         * @param config Configuration parameters
         */
        explicit ZMQServer(const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Bind to an endpoint
         *
         * Binds the server socket to the specified endpoint to accept
         * incoming connections.
         *
         * @param endpoint Bind address (e.g., "tcp://0.0.0.0:5555")
         * @return true on success, false on failure
         */
        bool bind(const std::string &endpoint);

        /**
         * @brief Send a LIMP frame
         *
         * Serializes and sends a LIMP frame via REP socket.
         *
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool send(const Frame &frame) override;

        /**
         * @brief Send response to client
         *
         * Sends a response to the most recent request. Must be called
         * after each receive() before receiving again.
         *
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return true on success, false on failure
         */
        bool send(const uint8_t *data, size_t size);

        /**
         * @brief Receive a LIMP frame
         *
         * Receives and deserializes a LIMP frame.
         *
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds
         * @return true on success, false on timeout or error
         */
        bool receive(Frame &frame, int timeoutMs = -1) override;

        /**
         * @brief Receive request from client
         *
         * Blocks until a request is received or timeout occurs.
         * Must be followed by a send() before receiving again.
         *
         * @param buffer Buffer to store received data
         * @param maxSize Maximum buffer size
         * @return Number of bytes received, 0 on timeout, -1 on error
         */
        std::ptrdiff_t receive(uint8_t *buffer, size_t maxSize);
    };

} // namespace limp
