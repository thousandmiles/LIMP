#pragma once

#include "../transport.hpp"
#include "zmq_config.hpp"
#include <zmq.hpp>
#include <memory>
#include <string>

namespace limp
{

    /**
     * @brief Base class for all ZeroMQ transport implementations
     *
     * Provides common functionality for managing ZeroMQ contexts, sockets,
     * and configuration. All ZeroMQ transport classes inherit from this base.
     *
     * This class handles:
     * - ZeroMQ context management
     * - Socket creation and configuration
     * - Error handling and callbacks
     * - Common socket options
     *
     * Thread safety: Not thread-safe. Use external synchronization if
     * accessing from multiple threads.
     */
    class ZMQTransport : public Transport
    {
    public:
        /**
         * @brief Construct a ZeroMQ transport with custom configuration
         *
         * @param config Configuration parameters for the transport
         */
        explicit ZMQTransport(const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Virtual destructor
         *
         * Ensures proper cleanup of derived classes. Closes socket and
         * terminates context.
         */
        virtual ~ZMQTransport();

        // Disable copy construction and assignment (sockets are unique resources)
        ZMQTransport(const ZMQTransport &) = delete;
        ZMQTransport &operator=(const ZMQTransport &) = delete;

        // Enable move operations (transfer ownership of socket/context)
        ZMQTransport(ZMQTransport &&) noexcept = default;
        ZMQTransport &operator=(ZMQTransport &&) noexcept = default;

        /**
         * @brief Check if transport is connected
         *
         * @return true if socket is created and endpoint is set, false otherwise
         */
        bool isConnected() const override;

        /**
         * @brief Close the transport connection
         *
         * Closes the ZeroMQ socket and cleans up resources. After calling
         * this method, the transport cannot be used until reconnected.
         */
        void close() override;

        /**
         * @brief Set error callback function
         *
         * Register a callback to be notified of transport errors. The callback
         * receives an error code and descriptive message.
         *
         * @param callback Function to call on errors
         */
        void setErrorCallback(ErrorCallback callback);

        /**
         * @brief Get the current endpoint
         *
         * @return The endpoint string (e.g., "tcp://127.0.0.1:5555")
         */
        const std::string &getEndpoint() const { return endpoint_; }

    protected:
        /**
         * @brief Create and configure a ZeroMQ socket
         *
         * Creates a socket of the specified type and applies all configuration
         * options from the config structure.
         *
         * @param socketType ZeroMQ socket type (ZMQ_REQ, ZMQ_REP, etc.)
         * @throws zmq::error_t if socket creation or configuration fails
         */
        void createSocket(zmq::socket_type socketType);

        /**
         * @brief Apply configuration options to the socket
         *
         * Sets all socket options based on the configuration structure.
         * Called automatically by createSocket().
         *
         * @throws zmq::error_t if option setting fails
         */
        void applySocketOptions();

        /**
         * @brief Handle ZeroMQ errors
         *
         * Logs error and invokes error callback if set. Used internally
         * to handle all ZeroMQ exceptions.
         *
         * @param e ZeroMQ exception
         * @param operation Description of the operation that failed
         */
        void handleError(const zmq::error_t &e, const std::string &operation);

        std::shared_ptr<zmq::context_t> context_; ///< Shared ZeroMQ context
        std::unique_ptr<zmq::socket_t> socket_;   ///< ZeroMQ socket
        ZMQConfig config_;                        ///< Transport configuration
        std::string endpoint_;                    ///< Connection endpoint
        ErrorCallback errorCallback_;             ///< Error notification callback
        bool connected_;                          ///< Connection state flag
    };

} // namespace limp
