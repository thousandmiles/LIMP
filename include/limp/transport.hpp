#pragma once

#include "frame.hpp"
#include <functional>
#include <memory>
#include <string>

namespace limp
{

    /**
     * Transport interface for sending/receiving LIMP frames
     * Abstract base class for different transport implementations (TCP, UDP, Serial, etc.)
     */
    class Transport
    {
    public:
        virtual ~Transport() = default;

        /**
         * Send a frame
         * @param frame Frame to send
         * @return true on success
         */
        virtual bool send(const Frame &frame) = 0;

        /**
         * Receive a frame (blocking or with timeout)
         * @param frame Output frame
         * @param timeoutMs Timeout in milliseconds (0 = non-blocking, -1 = infinite)
         * @return true if frame received
         */
        virtual bool receive(Frame &frame, int timeoutMs = -1) = 0;

        /**
         * Check if transport is connected/ready
         */
        virtual bool isConnected() const = 0;

        /**
         * Close the transport
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
