#include "limp/zmq/zmq_router.hpp"
#include <cstring>

namespace limp
{

    ZMQRouter::ZMQRouter(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::router);
    }

    bool ZMQRouter::bind(const std::string &endpoint)
    {
        if (!socket_)
        {
            return false;
        }

        try
        {
            socket_->bind(endpoint);
            endpoint_ = endpoint;
            connected_ = true;
            return true;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router bind");
            return false;
        }
    }

    std::ptrdiff_t ZMQRouter::receive(std::vector<uint8_t> &identity,
                                      uint8_t *buffer,
                                      size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
            // Receive identity frame
            zmq::message_t identityMsg;
            auto identityResult = socket_->recv(identityMsg, zmq::recv_flags::none);
            if (!identityResult)
            {
                return 0; // Timeout
            }

            // Store identity
            identity.assign(static_cast<uint8_t *>(identityMsg.data()),
                            static_cast<uint8_t *>(identityMsg.data()) + identityMsg.size());

            // Receive delimiter frame (empty frame in ROUTER-DEALER pattern)
            zmq::message_t delimiterMsg;
            auto delimiterResult = socket_->recv(delimiterMsg, zmq::recv_flags::none);
            if (!delimiterResult)
            {
                handleError(zmq::error_t(), "router receive: missing delimiter frame");
                return -1;
            }

            // Receive data frame
            zmq::message_t dataMsg;
            auto dataResult = socket_->recv(dataMsg, zmq::recv_flags::none);
            if (!dataResult)
            {
                handleError(zmq::error_t(), "router receive: missing data frame");
                return -1;
            }

            size_t receivedSize = dataMsg.size();
            if (receivedSize > maxSize)
            {
                handleError(zmq::error_t(), "received message larger than buffer");
                return -1;
            }

            std::memcpy(buffer, dataMsg.data(), receivedSize);
            return static_cast<std::ptrdiff_t>(receivedSize);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router receive");
            return -1;
        }
    }

    bool ZMQRouter::receive(std::vector<uint8_t> &identity,
                            Frame &frame,
                            int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> buffer(4096); // Reasonable default size
        std::ptrdiff_t received = receive(identity, buffer.data(), buffer.size());

        if (received <= 0)
        {
            return false;
        }

        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
    }

    bool ZMQRouter::send(const std::vector<uint8_t> &identity,
                         const uint8_t *data,
                         size_t size)
    {
        if (!isConnected())
        {
            return false;
        }

        try
        {
            // Send identity frame
            zmq::message_t identityMsg(identity.data(), identity.size());
            auto identityResult = socket_->send(identityMsg, zmq::send_flags::sndmore);
            if (!identityResult)
            {
                return false;
            }

            // Send delimiter frame (empty)
            zmq::message_t delimiterMsg;
            auto delimiterResult = socket_->send(delimiterMsg, zmq::send_flags::sndmore);
            if (!delimiterResult)
            {
                return false;
            }

            // Send data frame
            zmq::message_t dataMsg(data, size);
            auto dataResult = socket_->send(dataMsg, zmq::send_flags::none);
            return dataResult.has_value();
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router send");
            return false;
        }
    }

    bool ZMQRouter::send(const std::vector<uint8_t> &identity,
                         const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }
        return send(identity, buffer.data(), buffer.size());
    }

    bool ZMQRouter::send(const Frame &frame)
    {
        (void)frame;
        handleError(zmq::error_t(), "router requires client identity for send");
        return false;
    }

    bool ZMQRouter::receive(Frame &frame, int timeoutMs)
    {
        (void)frame;
        (void)timeoutMs;
        handleError(zmq::error_t(), "router requires identity output for receive");
        return false;
    }

} // namespace limp
