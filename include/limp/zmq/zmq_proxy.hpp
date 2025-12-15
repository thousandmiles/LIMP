#pragma once

#include "zmq_config.hpp"
#include <zmq.hpp>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <functional>

namespace limp
{

    /**
     * @brief ZeroMQ proxy for message forwarding and brokering
     *
     * Implements a message proxy/broker that forwards messages between
     * frontend and backend sockets. Runs in a separate thread and provides
     * common patterns like load balancing and message routing.
     *
     * PROXY PATTERNS OVERVIEW:
     *
     * 1. ROUTER-DEALER (Load Balancer Pattern)
     *    Purpose: Distribute client requests to available workers
     *    Frontend: ROUTER socket (binds) - receives from multiple clients
     *    Backend: DEALER socket (binds) - distributes to multiple workers
     *    Flow: Client (REQ/DEALER) → ROUTER → DEALER → Worker (REP/DEALER)
     *    Behavior: Load balances requests using LRU (Least Recently Used)
     *    Use Case: Web server backends, job processing, task distribution
     *
     * 2. ROUTER-ROUTER (Message Broker Pattern)
     *    Purpose: Central message broker for N:N node communication
     *    Frontend: ROUTER socket (binds) - multiple nodes connect
     *    Backend: ROUTER socket (binds) - more nodes connect (or same endpoint)
     *    Flow: Any Node (DEALER) → ROUTER → ROUTER → Any Node (DEALER)
     *    Behavior: Routes messages by client identity
     *    Use Case: Industrial systems (HMI, SCADA, PLC communication)
     *    Example Nodes: HMI, SCADA, Logger, PLC, Historian, Alarm systems
     *
     * 3. DEALER-DEALER (Pipeline Pattern)
     *    Purpose: Asynchronous task distribution pipeline
     *    Frontend: DEALER socket (binds) - producers connect
     *    Backend: DEALER socket (binds) - consumers connect
     *    Flow: Producer (DEALER) → DEALER → DEALER → Consumer (DEALER)
     *    Behavior: Fair-queued distribution, fully asynchronous
     *    Use Case: Task pipelines, data processing streams
     *
     * 4. XPUB-XSUB (Pub/Sub Forwarder Pattern)
     *    Purpose: Centralized publish-subscribe message bus
     *    Frontend: XSUB socket (binds) - publishers connect
     *    Backend: XPUB socket (binds) - subscribers connect
     *    Flow: Publisher (PUB) → XSUB → XPUB → Subscriber (SUB)
     *    Behavior: Topic-based filtering and forwarding
     *    Use Case: Event distribution, sensor data, alarm systems
     *    Example: Sensors, Alarms, Events → Monitors, Loggers, Dashboards
     *
     * Pattern Comparison:
     *
     *   Pattern        | Use Case         | Load Balance | Routing
     *   ----------------------------------------------------------------
     *   ROUTER-DEALER  | Request/Work     | Yes (LRU)    | To available
     *   ROUTER-ROUTER  | Message Broker   | No           | By identity
     *   DEALER-DEALER  | Pipeline         | Yes (fair)   | Fair-queued
     *   XPUB-XSUB      | Pub/Sub Bus      | No           | By topic
     *
     * Supported patterns:
     * - ROUTER-DEALER: Load balancer (clients to workers)
     * - ROUTER-ROUTER: Message broker (bidirectional routing)
     * - DEALER-DEALER: Pipeline (async forwarding)
     * - XPUB-XSUB: Pub/Sub forwarder
     *
     * The proxy automatically forwards all messages between frontend and
     * backend sockets using ZeroMQ's built-in proxy functionality.
     *
     * Example usage:
     * @code
     * // Create a load balancer
     * ZMQProxy proxy(ZMQProxy::ProxyType::ROUTER_DEALER);
     * proxy.setFrontend("tcp://0.0.0.0:5555", true);   // Clients connect here
     * proxy.setBackend("tcp://0.0.0.0:5556", true);    // Workers connect here
     * proxy.setCapture("tcp://0.0.0.0:9999");          // Optional monitoring
     * proxy.start();
     *
     * // Proxy runs in background...
     * // When done:
     * proxy.stop();
     * @endcode
     */
    class ZMQProxy
    {
    public:
        /**
         * @brief Proxy type enumeration
         */
        enum class ProxyType
        {
            ROUTER_DEALER, ///< Load balancer: ROUTER frontend, DEALER backend
            ROUTER_ROUTER, ///< Message broker: ROUTER on both sides
            DEALER_DEALER, ///< Pipeline: DEALER on both sides
            XPUB_XSUB      ///< Pub/Sub forwarder: XPUB frontend, XSUB backend
        };

        /**
         * @brief Construct a ZeroMQ proxy
         *
         * @param type Type of proxy pattern to implement
         * @param config Configuration parameters for sockets
         */
        explicit ZMQProxy(ProxyType type, const ZMQConfig &config = ZMQConfig());

        /**
         * @brief Destructor
         *
         * Automatically stops the proxy if running.
         */
        ~ZMQProxy();

        // Disable copy construction and assignment
        ZMQProxy(const ZMQProxy &) = delete;
        ZMQProxy &operator=(const ZMQProxy &) = delete;

        /**
         * @brief Set frontend endpoint
         *
         * Configures the frontend socket endpoint. Must be called before start().
         *
         * @param endpoint Endpoint address (e.g., "tcp://0.0.0.0:5555")
         * @param bind If true, binds to endpoint; if false, connects to endpoint
         * @return true on success, false if already running
         */
        bool setFrontend(const std::string &endpoint, bool bind = true);

        /**
         * @brief Set backend endpoint
         *
         * Configures the backend socket endpoint. Must be called before start().
         *
         * @param endpoint Endpoint address (e.g., "tcp://0.0.0.0:5556")
         * @param bind If true, binds to endpoint; if false, connects to endpoint
         * @return true on success, false if already running
         */
        bool setBackend(const std::string &endpoint, bool bind = true);

        /**
         * @brief Set capture endpoint for monitoring
         *
         * Optionally configures a capture socket to monitor all traffic
         * passing through the proxy. The capture socket is a PUB socket
         * that publishes all messages.
         *
         * @param endpoint Capture endpoint (e.g., "tcp://0.0.0.0:9999")
         * @return true on success, false if already running
         */
        bool setCapture(const std::string &endpoint);

        /**
         * @brief Set error callback function
         *
         * Register a callback to be notified of proxy errors.
         *
         * @param callback Function to call on errors
         */
        void setErrorCallback(std::function<void(const std::string &)> callback);

        /**
         * @brief Start the proxy
         *
         * Starts the proxy in a background thread. The proxy will run
         * until stop() is called or an error occurs.
         *
         * @return true if started successfully, false if already running or config invalid
         */
        bool start();

        /**
         * @brief Stop the proxy
         *
         * Stops the proxy thread and cleans up resources. This is a
         * blocking call that waits for the thread to terminate.
         */
        void stop();

        /**
         * @brief Check if proxy is running
         *
         * @return true if proxy thread is active, false otherwise
         */
        bool isRunning() const { return running_.load(); }

        /**
         * @brief Get frontend endpoint
         *
         * @return The frontend endpoint string
         */
        const std::string &getFrontendEndpoint() const { return frontendEndpoint_; }

        /**
         * @brief Get backend endpoint
         *
         * @return The backend endpoint string
         */
        const std::string &getBackendEndpoint() const { return backendEndpoint_; }

    private:
        /**
         * @brief Proxy thread main function
         *
         * Runs the ZeroMQ proxy loop in a separate thread.
         */
        void proxyThread();

        /**
         * @brief Handle error with callback
         *
         * @param error ZeroMQ error object
         * @param context Context string for error message
         */
        void handleError(const zmq::error_t &error, const std::string &context);

        /**
         * @brief Get socket type for frontend based on proxy type
         */
        zmq::socket_type getFrontendSocketType() const;

        /**
         * @brief Get socket type for backend based on proxy type
         */
        zmq::socket_type getBackendSocketType() const;

        ProxyType type_;                                           ///< Proxy pattern type
        ZMQConfig config_;                                         ///< Socket configuration
        std::unique_ptr<zmq::context_t> context_;                  ///< ZeroMQ context
        std::unique_ptr<std::thread> thread_;                      ///< Proxy thread
        std::atomic<bool> running_;                                ///< Running flag
        std::atomic<bool> stopRequested_;                          ///< Stop request flag
        std::string frontendEndpoint_;                             ///< Frontend endpoint
        std::string backendEndpoint_;                              ///< Backend endpoint
        std::string captureEndpoint_;                              ///< Capture endpoint (optional)
        bool frontendBind_;                                        ///< Bind (true) or connect (false) frontend
        bool backendBind_;                                         ///< Bind (true) or connect (false) backend
        std::function<void(const std::string &)> errorCallback_;   ///< Error callback
    };

} // namespace limp
