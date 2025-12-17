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
         * @brief Receive a LIMP frame with source identity only
         *
         * Receives a frame sent by dealer.send(frame) without destination routing.
         * DEALER sends: [delimiter][data] (2 parts)
         * ROUTER receives: [dealer_identity][delimiter][data] (3 parts, identity added by ZMQ)
         *
         * Pair with: dealer.send(frame)
         *
         * @param sourceIdentity Output: sender's identity
         * @param frame Output: received frame
         * @param timeoutMs Timeout in milliseconds (currently unused, uses socket config timeout)
         * @return true on success, false on timeout or error
         */
        bool receive(std::string &sourceIdentity,
                     Frame &frame,
                     int timeoutMs = -1);

        /**
         * @brief Receive a LIMP frame with source and destination identities
         *
         * Receives a frame sent by dealer.send(dest, frame) with destination routing.
         * DEALER sends: [dest_identity][delimiter][data] (3 parts)
         * ROUTER receives: [dealer_identity][dest_identity][delimiter][data] (4 parts)
         *
         * Pair with: dealer.send(destinationIdentity, frame)
         *
         * @param sourceIdentity Output: sender's identity
         * @param destinationIdentity Output: intended recipient's identity
         * @param frame Output: received frame
         * @param timeoutMs Timeout in milliseconds (currently unused, uses socket config timeout)
         * @return true on success, false on timeout or error
         */
        bool receive(std::string &sourceIdentity,
                     std::string &destinationIdentity,
                     Frame &frame,
                     int timeoutMs = -1);

        /**
         * @brief Send a LIMP frame to a specific client without source identity
         *
         * Sends a frame to the specified client.
         * ROUTER sends: [client_identity][delimiter][data] (3 parts)
         * DEALER receives: [delimiter][data] (2 parts, identity stripped by ZMQ)
         *
         * Pair with: dealer.receive(frame)
         *
         * @param clientIdentity Target client identity
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool send(const std::string &clientIdentity, const Frame &frame);

        /**
         * @brief Send a LIMP frame to a specific client with source identity
         *
         * Sends a frame to the specified client including source identity for tracking.
         * ROUTER sends: [client_identity][source_identity][delimiter][data] (4 parts)
         * DEALER receives: [source_identity][delimiter][data] (3 parts, client identity stripped by ZMQ)
         *
         * Pair with: dealer.receive(sourceIdentity, frame)
         *
         * @param clientIdentity Target client identity
         * @param sourceIdentity Source identity representing the sender
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool send(const std::string &clientIdentity, const std::string &sourceIdentity, const Frame &frame);

        /**
         * @brief Not supported for router (use identity-based send)
         * @return false
         */
        bool send(const Frame &frame) override;

        /**
         * @brief Not supported for router (use identity-based receive)
         * @param timeoutMs Ignored
         * @return false
         */
        bool receive(Frame &frame, int timeoutMs = -1) override;

        /**
         * @brief Receive raw data without destination routing
         *
         * DEALER sends: [delimiter][data] (2 parts)
         * ROUTER receives: [dealer_identity][delimiter][data] (3 parts, identity added by ZMQ)
         *
         * Pair with: dealer.sendRaw(data, size)
         *
         * @param identity Output: sender's identity
         * @param buffer Pointer to buffer to store received data
         * @param maxSize Maximum size of the buffer
         * @return Number of bytes received, or -1 on error
         */
        std::ptrdiff_t receiveRaw(std::vector<uint8_t> &identity,
                                  uint8_t *buffer,
                                  size_t maxSize);

        /**
         * @brief Receive raw data with destination routing
         *
         * DEALER sends: [dest_identity][delimiter][data] (3 parts)
         * ROUTER receives: [dealer_identity][dest_identity][delimiter][data] (4 parts, identity added by ZMQ)
         *
         * Pair with: dealer.sendRaw(destinationIdentity, data, size)
         *
         * @param sourceIdentity Output: sender's identity
         * @param destinationIdentity Output: intended recipient's identity
         * @param buffer Pointer to buffer to store received data
         * @param maxSize Maximum size of the buffer
         * @return Number of bytes received, or -1 on error
         */
        std::ptrdiff_t receiveRaw(std::vector<uint8_t> &sourceIdentity,
                                  std::vector<uint8_t> &destinationIdentity,
                                  uint8_t *buffer,
                                  size_t maxSize);

        /**
         * @brief Send raw data without source identity
         *
         * ROUTER sends: [client_identity][delimiter][data] (3 parts)
         * DEALER receives: [delimiter][data] (2 parts, identity stripped by ZMQ)
         *
         * Pair with: dealer.receiveRaw(buffer, maxSize)
         *
         * @param clientIdentity Target client identity
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return true on success, false on failure
         */
        bool sendRaw(const std::vector<uint8_t> &clientIdentity,
                     const uint8_t *data,
                     size_t size);

        /**
         * @brief Send raw data with source identity
         *
         * ROUTER sends: [client_identity][source_identity][delimiter][data] (4 parts)
         * DEALER receives: [source_identity][delimiter][data] (3 parts, client_identity stripped by ZMQ)
         *
         * Pair with: dealer.receiveRaw(sourceIdentity, buffer, maxSize)
         *
         * @param clientIdentity Target client identity
         * @param sourceIdentity Source identity representing the sender
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return true on success, false on failure
         */
        bool sendRaw(const std::vector<uint8_t> &clientIdentity,
                     const std::vector<uint8_t> &sourceIdentity,
                     const uint8_t *data,
                     size_t size);
    };

} // namespace limp
