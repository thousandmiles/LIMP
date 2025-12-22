#pragma once

#include "frame.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace limp
{

    /**
     * @brief Infrastructure-level transport error codes
     *
     * Represents errors during frame transmission/reception at the transport layer.
     * These are distinct from application-level errors (MsgType::ERROR).
     * Returned by all Transport implementations.
     */
    enum class TransportError
    {
        None = 0,              ///< No error (success)
        ConnectionFailed,      ///< Failed to establish connection
        BindFailed,            ///< Failed to bind to endpoint
        SendFailed,            ///< Failed to send data
        ReceiveFailed,         ///< Failed to receive data
        Timeout,               ///< Operation timed out
        InvalidEndpoint,       ///< Invalid endpoint format
        SocketClosed,          ///< Socket is closed
        NotConnected,          ///< Not connected to endpoint
        SerializationFailed,   ///< Failed to serialize frame
        DeserializationFailed, ///< Failed to deserialize frame
        InvalidFrame,          ///< Frame validation failed
        AlreadyConnected,      ///< Already connected/bound
        ConfigurationError,    ///< Invalid configuration
        InternalError          ///< Unspecified internal error
    };

    /**
     * @brief Convert TransportError to human-readable string
     * @param error Transport error code
     * @return Error description string (never null)
     */
    const char *toString(TransportError error) noexcept;

    /**
     * @brief Abstract transport interface for LIMP frames
     *
     * Base class for all transport implementations (ZeroMQ, TCP, UDP, Serial, etc.).
     * Provides consistent send/receive API with infrastructure-level error handling.
     *
     * Method hierarchy:
     * - send(Frame) and receive(Frame) are the primary abstract methods
     * - sendRaw() and receiveRaw() are optional methods for raw data access
     * - Frame-based methods typically call Raw methods internally after serialization
     */
    class Transport
    {
    public:
        virtual ~Transport() = default;

        /**
         * @brief Send a frame over transport
         *
         * Primary method for sending LIMP frames. Handles serialization internally.
         *
         * @param frame Frame to transmit
         * @return TransportError::None on success, error code on failure
         */
        virtual TransportError send(const Frame &frame) = 0;

        /**
         * @brief Send raw data over transport
         *
         * Low-level method for sending raw bytes without frame serialization.
         * Not all transports need to expose this.
         *
         * @param data Pointer to data buffer
         * @param size Size of data in bytes
         * @return TransportError::None on success, error code on failure
         */
        virtual TransportError sendRaw(const uint8_t *data, size_t size)
        {
            (void)data;
            (void)size;
            return TransportError::InternalError; // Not supported by default
        }

        /**
         * @brief Receive a frame from transport
         *
         * Primary method for receiving LIMP frames. Handles deserialization internally.
         *
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds (0=non-blocking, -1=infinite)
         * @return TransportError::None on success, TransportError::Timeout on timeout,
         *         other error code on failure
         */
        virtual TransportError receive(Frame &frame, int timeoutMs = -1) = 0;

        /**
         * @brief Receive raw data from transport
         *
         * Low-level method for receiving raw bytes without frame deserialization.
         * Not all transports need to expose this.
         *
         * @param buffer Pointer to buffer to store received data
         * @param maxSize Maximum size of the buffer
         * @return Number of bytes received, 0 on timeout, or -1 on error
         */
        virtual std::ptrdiff_t receiveRaw(uint8_t *buffer, size_t maxSize)
        {
            (void)buffer;
            (void)maxSize;
            return -1; // Not supported by default
        }

        /**
         * @brief Check if transport is connected and ready
         * @return true if connected, false otherwise
         */
        virtual bool isConnected() const = 0;

        /**
         * @brief Close the transport connection
         */
        virtual void close() = 0;
    };

    /**
     * Callback type for asynchronous frame reception
     */
    using FrameCallback = std::function<void(const Frame &)>;

    /**
     * Error callback type
     */
    using ErrorCallback = std::function<void(const std::string &)>;

} // namespace limp
