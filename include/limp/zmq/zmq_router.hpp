#pragma once

#include "zmq_transport_base.hpp"
#include <cstddef>
#include <vector>

namespace limp
{

    /**
     * @brief ZeroMQ router transport using ROUTER socket
     *
     * Implements an asynchronous routing server that can communicate with
     * multiple DEALER clients. The ROUTER socket automatically tracks client
     * identities and allows selective message routing.
     *
     * ROUTER PATTERN OVERVIEW:
     *
     * Communication Flow:
     *   1. DEALER Client 1 (identity: A) sends message to ROUTER
     *   2. ROUTER receives [identity A][delimiter][data]
     *   3. ROUTER processes and determines routing
     *   4. ROUTER sends response to specific client by identity
     *   5. Client B receives the routed message
     *
     * Example Scenario:
     *   - Client A sends request to ROUTER server
     *   - ROUTER automatically captures client A's identity
     *   - ROUTER processes request and decides to forward to Client B
     *   - ROUTER sends [identity B][delimiter][response] to Client B
     *   - Client B receives the response
     *
     * Key Characteristics:
     *   - ROUTER binds (server), DEALER connects (client)
     *   - ROUTER automatically tracks client identities
     *   - Can route messages to specific clients by identity
     *   - No send-receive order enforcement (fully asynchronous)
     *   - Supports N:1 communication (many clients to one server)
     *   - Each message includes identity frame for routing
     *
     * Key features:
     * - Receives messages with client identity frames
     * - Can route responses to specific clients by identity
     * - No strict send-receive alternation (unlike REQ-REP)
     * - Supports multiple simultaneous connections
     * - Ideal for broker/proxy/load balancer patterns
     *
     * Message format (multipart):
     * [identity frame][delimiter frame][data frames...]
     *
     * Example usage:
     * @code
     * ZMQRouter router;
     * router.bind("tcp://0.0.0.0:5555");
     *
     * // Receive message with client identity
     * std::vector<uint8_t> clientId;
     * Frame frame;
     * router.receive(clientId, frame);
     *
     * // Process and route back to specific client
     * router.send(clientId, responseFrame);
     * @endcode
     */
    class ZMQRouter : public ZMQTransport
    {
    public:
        /**
         * @brief Construct a ZeroMQ router
         *
         * @param config Configuration parameters
         */
        explicit ZMQRouter(const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Bind to an endpoint
         *
         * Binds the router socket to the specified endpoint to accept
         * incoming connections from DEALER clients.
         *
         * @param endpoint Bind address (e.g., "tcp://0.0.0.0:5555")
         * @return true on success, false on failure
         */
        bool bind(const std::string &endpoint);

        /**
         * @brief Receive a message with client identity
         *
         * Receives a multipart message from a DEALER client. The first
         * frame contains the client's identity, followed by the data.
         *
         * @param identity Output vector to store client identity
         * @param buffer Buffer to store received data
         * @param maxSize Maximum buffer size
         * @return Number of bytes received (data only), 0 on timeout, -1 on error
         */
        std::ptrdiff_t receive(std::vector<uint8_t> &identity,
                               uint8_t *buffer,
                               size_t maxSize);

        /**
         * @brief Receive a LIMP frame with client identity
         *
         * Receives and deserializes a LIMP frame along with the client identity.
         *
         * @param identity Output vector to store client identity
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds (uses socket config if -1)
         * @return true on success, false on timeout or error
         */
        bool receive(std::vector<uint8_t> &identity,
                     Frame &frame,
                     int timeoutMs = -1);

        /**
         * @brief Send data to a specific client
         *
         * Sends a message to the client identified by the given identity.
         * The identity should be obtained from a previous receive() call.
         *
         * @param identity Client identity to send to
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return true on success, false on failure
         */
        bool send(const std::vector<uint8_t> &identity,
                  const uint8_t *data,
                  size_t size);

        /**
         * @brief Send a LIMP frame to a specific client
         *
         * Serializes and sends a LIMP frame to the specified client.
         *
         * @param identity Client identity to send to
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool send(const std::vector<uint8_t> &identity,
                  const Frame &frame);

        /**
         * @brief Not supported for router (use identity-based send)
         * @return false
         */
        bool send(const Frame &frame) override;

        /**
         * @brief Not supported for router (use identity-based receive)
         * @return false
         */
        bool receive(Frame &frame, int timeoutMs = -1) override;
    };

} // namespace limp
