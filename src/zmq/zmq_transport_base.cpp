#include "limp/zmq/zmq_transport_base.hpp"
#include <iostream>

namespace limp
{

    ZMQTransport::ZMQTransport(const ZMQConfig &config)
        : config_(config), connected_(false)
    {
        try
        {
            context_ = std::make_shared<zmq::context_t>(config_.ioThreads);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "context creation");
            throw;
        }
    }

    ZMQTransport::~ZMQTransport()
    {
        close();
    }

    bool ZMQTransport::isConnected() const
    {
        return connected_ && socket_ != nullptr;
    }

    void ZMQTransport::close()
    {
        if (socket_)
        {
            try
            {
                socket_->close();
            }
            catch (const zmq::error_t &e)
            {
                handleError(e, "socket close");
            }
            socket_.reset();
        }
        connected_ = false;
        endpoint_.clear();
    }

    void ZMQTransport::setErrorCallback(ErrorCallback callback)
    {
        errorCallback_ = std::move(callback);
    }

    void ZMQTransport::createSocket(zmq::socket_type socketType)
    {
        try
        {
            socket_ = std::make_unique<zmq::socket_t>(*context_, socketType);
            applySocketOptions();
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "socket creation");
            throw;
        }
    }

    void ZMQTransport::applySocketOptions()
    {
        if (!socket_)
            return;

        try
        {
            // Set send timeout
            socket_->set(zmq::sockopt::sndtimeo, config_.sendTimeout);

            // Set receive timeout
            socket_->set(zmq::sockopt::rcvtimeo, config_.receiveTimeout);

            // Set linger time
            socket_->set(zmq::sockopt::linger, config_.lingerTime);

            // Set send buffer size if specified
            if (config_.sendBufferSize > 0)
            {
                socket_->set(zmq::sockopt::sndbuf, config_.sendBufferSize);
            }

            // Set receive buffer size if specified
            if (config_.receiveBufferSize > 0)
            {
                socket_->set(zmq::sockopt::rcvbuf, config_.receiveBufferSize);
            }

            // Set reconnect interval
            socket_->set(zmq::sockopt::reconnect_ivl, config_.reconnectInterval);

            // Set maximum reconnect interval if specified
            if (config_.reconnectIntervalMax > 0)
            {
                socket_->set(zmq::sockopt::reconnect_ivl_max, config_.reconnectIntervalMax);
            }

            // Set immediate mode
            socket_->set(zmq::sockopt::immediate, config_.immediate ? 1 : 0);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "socket option setting");
            throw;
        }
    }

    void ZMQTransport::handleError(const zmq::error_t &e, const std::string &operation)
    {
        std::string errorMsg = "ZMQ error during " + operation + ": " + e.what() + " (code: " + std::to_string(e.num()) + ")";

        if (errorCallback_)
        {
            errorCallback_(errorMsg);
        }
        else
        {
            std::cerr << errorMsg << std::endl;
        }
    }

} // namespace limp
