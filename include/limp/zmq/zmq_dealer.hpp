#pragma once

#include "zmq_transport_base.hpp"
#include <cstddef>
#include <string>

namespace limp
{

    /**
     * @brief ZeroMQ dealer transport using DEALER socket
     *
     * Implements an asynchronous client that can communicate with ROUTER
     * sockets. Unlike REQ sockets, DEALER does not enforce strict send-receive
     * alternation, allowing for more flexible messaging patterns.
     *
     * DEALER PATTERN OVERVIEW:
     *
     * Communication Flow:
     *   1. DEALER connects to ROUTER server
     *   2. DEALER can send multiple messages without waiting for replies
     *   3. ROUTER receives messages with DEALER's identity attached
     *   4. ROUTER can send responses at any time
     *   5. DEALER receives responses asynchronously
     *
     * Asynchronous Behavior:
     *   - DEALER can call send() multiple times in a row
     *   - DEALER can call receive() multiple times in a row
     *   - No enforced alternation between send and receive
     *   - Messages can be queued on both sides
     *
     * Comparison with REQ/REP Pattern:
     *
     *   REQ (Synchronous - Strict Order):
     *     - Must alternate: send() -> receive() -> send() -> receive()
     *     - Cannot send twice without receiving in between
     *     - Enforced state machine (send state vs receive state)
     *
     *   DEALER (Asynchronous - Flexible Order):
     *     - Can send() multiple times before any receive()
     *     - Can receive() multiple times without sending
     *     - No state machine enforcement
     *     - Full asynchronous operation
     *
     * Key features:
     * - Asynchronous send/receive (no strict alternation required)
     * - Can connect to ROUTER sockets
     * - Automatic or manual identity assignment
     * - Multiple dealers can connect to one router
     * - Ideal for worker pools, async clients, and distributed systems
     *
     * Example usage:
     * @code
     * ZMQDealer dealer;
     * dealer.setIdentity("worker-001");
     * dealer.connect("tcp://127.0.0.1:5555");
     *
     * // Send request (no need to receive immediately)
     * dealer.send(requestData);
     *
     * // Can send multiple requests
     * dealer.send(anotherRequest);
     *
     * // Receive when ready
     * Frame response;
     * dealer.receive(response);
     * @endcode
     */
    class ZMQDealer : public ZMQTransport
    {
    public:
        /**
         * @brief Construct a ZeroMQ dealer
         *
         * @param config Configuration parameters
         */
        explicit ZMQDealer(const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Set socket identity
         *
         * Sets a custom identity for this DEALER socket. Must be called
         * before connect(). If not set, ZeroMQ generates a random UUID.
         *
         * Note: Identity must be unique within the context of the ROUTER.
         *
         * @param identity Unique identity string
         * @return true on success, false if already connected
         */
        bool setIdentity(const std::string &identity);

        /**
         * @brief Connect to a router endpoint
         *
         * Establishes connection to the specified ROUTER endpoint.
         *
         * @param endpoint Router address (e.g., "tcp://127.0.0.1:5555")
         * @return true on success, false on failure
         */
        bool connect(const std::string &endpoint);

        /**
         * @brief Send data to the connected router
         *
         * Sends a message to the router. Does not require prior receive
         * or subsequent receive (asynchronous operation).
         *
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return true on success, false on failure
         */
        bool send(const uint8_t *data, size_t size);

        /**
         * @brief Send a LIMP frame
         *
         * Serializes and sends a LIMP frame via DEALER socket.
         *
         * @param frame Frame to send
         * @return true on success, false on failure
         */
        bool send(const Frame &frame) override;

        /**
         * @brief Receive data from router
         *
         * Blocks until a message is received or timeout occurs.
         * Can be called at any time (no prior send required).
         *
         * @param buffer Buffer to store received data
         * @param maxSize Maximum buffer size
         * @return Number of bytes received, 0 on timeout, -1 on error
         */
        std::ptrdiff_t receive(uint8_t *buffer, size_t maxSize);

        /**
         * @brief Receive a LIMP frame
         *
         * Receives and deserializes a LIMP frame.
         *
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds (uses socket config if -1)
         * @return true on success, false on timeout or error
         */
        bool receive(Frame &frame, int timeoutMs = -1) override;

        /**
         * @brief Get the current identity
         *
         * @return The identity string, or empty if using auto-generated identity
         */
        const std::string &getIdentity() const { return identity_; }

    private:
        std::string identity_; ///< Socket identity
    };

} // namespace limp
