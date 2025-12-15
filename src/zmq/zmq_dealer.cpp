#include "limp/zmq/zmq_dealer.hpp"
#include <cstring>

namespace limp
{

    ZMQDealer::ZMQDealer(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::dealer);
    }

    bool ZMQDealer::setIdentity(const std::string &identity)
    {
        if (connected_)
        {
            handleError(zmq::error_t(), "cannot set identity after connection");
            return false;
        }

        if (!socket_)
        {
            return false;
        }

        try
        {
            socket_->set(zmq::sockopt::routing_id, identity);
            identity_ = identity;
            return true;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer set identity");
            return false;
        }
    }

    bool ZMQDealer::connect(const std::string &endpoint)
    {
        if (!socket_)
        {
            return false;
        }

        try
        {
            socket_->connect(endpoint);
            endpoint_ = endpoint;
            connected_ = true;
            return true;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer connect");
            return false;
        }
    }

    bool ZMQDealer::send(const uint8_t *data, size_t size)
    {
        if (!isConnected())
        {
            return false;
        }

        try
        {
            // DEALER sends data directly (no identity frame needed on send)
            zmq::message_t message(data, size);
            auto result = socket_->send(message, zmq::send_flags::none);
            return result.has_value();
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer send");
            return false;
        }
    }

    std::ptrdiff_t ZMQDealer::receive(uint8_t *buffer, size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
            // DEALER receives data directly (no identity frame on receive)
            zmq::message_t message;
            auto result = socket_->recv(message, zmq::recv_flags::none);

            if (!result)
            {
                return 0; // Timeout
            }

            size_t receivedSize = message.size();
            if (receivedSize > maxSize)
            {
                handleError(zmq::error_t(), "received message larger than buffer");
                return -1;
            }

            std::memcpy(buffer, message.data(), receivedSize);
            return static_cast<std::ptrdiff_t>(receivedSize);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer receive");
            return -1;
        }
    }

    bool ZMQDealer::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }
        return send(buffer.data(), buffer.size());
    }

    bool ZMQDealer::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> buffer(4096); // Reasonable default size
        std::ptrdiff_t received = receive(buffer.data(), buffer.size());

        if (received <= 0)
        {
            return false;
        }

        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
    }

} // namespace limp
