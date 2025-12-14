#pragma once

#include <functional>
#include <string>

namespace limp
{

    /**
     * @brief Configuration structure for ZeroMQ transport
     *
     * Contains all configuration parameters for creating and configuring
     * ZeroMQ sockets, including timeouts, buffer sizes, and connection options.
     */
    struct ZMQConfig
    {
        int sendTimeout = 1000;       ///< Send timeout in milliseconds (-1 for infinite)
        int receiveTimeout = 1000;    ///< Receive timeout in milliseconds (-1 for infinite)
        int lingerTime = 0;           ///< Linger time on socket close in milliseconds
        int sendBufferSize = 0;       ///< Send buffer size in bytes (0 for default)
        int receiveBufferSize = 0;    ///< Receive buffer size in bytes (0 for default)
        int reconnectInterval = 100;  ///< Reconnection interval in milliseconds
        int reconnectIntervalMax = 0; ///< Maximum reconnection interval (0 for default)
        bool immediate = true;        ///< Queue messages only to completed connections
        int ioThreads = 1;            ///< Number of I/O threads in ZMQ context
    };

} // namespace limp
