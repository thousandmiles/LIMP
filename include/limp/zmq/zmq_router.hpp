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
         * @brief Receive a LIMP frame from client
         *
         * Receives and deserializes a LIMP frame along with the sender's identity.
         * Use this when clients send messages without destination (dealer.send()).
         *
         * @param sourceIdentity Output: sender's identity
         * @param frame Output: received frame
         * @param timeoutMs Timeout in milliseconds (uses socket config if -1)
         * @return true on success, false on timeout or error
         */
        bool receive(std::string &sourceIdentity,
                     Frame &frame,
                     int timeoutMs = -1);

        /**
         * @brief Receive a LIMP frame with source and destination identities
         *
         * Receives and deserializes a LIMP frame, extracting both the sender's
         * identity and the intended destination identity. Use this when clients
         * send messages with destination (dealer.sendTo(dest, frame)).
         *
         * @param sourceIdentity Output: sender's identity
         * @param destinationIdentity Output: intended recipient's identity (empty if not specified)
         * @param frame Output: received frame
         * @param timeoutMs Timeout in milliseconds (uses socket config if -1)
         * @return true on success, false on timeout or error
         */
        bool receive(std::string &sourceIdentity,
                     std::string &destinationIdentity,
                     Frame &frame,
                     int timeoutMs = -1);

        /**
         * @brief Send a LIMP frame to a specific client
         *
         * Serializes and sends a LIMP frame to the client with the specified identity.
         * The identity should be obtained from a previous receive() call.
         *
         * @param clientIdentity Client identity to send to
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool send(const std::string &clientIdentity, const Frame &frame);

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

    private:
        /**
         * @brief Internal helper to receive raw multipart message
         */
        std::ptrdiff_t receiveRaw(std::vector<uint8_t> &identity,
                                  uint8_t *buffer,
                                  size_t maxSize);

        /**
         * @brief Internal helper to receive raw data for 5-frame routing messages
         */
        std::ptrdiff_t receiveRaw(std::vector<uint8_t> &sourceIdentity,
                                  std::vector<uint8_t> &destinationIdentity,
                                  uint8_t *buffer,
                                  size_t maxSize);

        /**
         * @brief Internal helper to send raw data to client
         */
        bool sendRaw(const std::vector<uint8_t> &identity,
                     const uint8_t *data,
                     size_t size);
    };

} // namespace limp
