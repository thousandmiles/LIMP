#include "limp/zmq/zmq_client.hpp"

namespace limp
{

    ZMQClient::ZMQClient(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::req);
    }

    bool ZMQClient::connect(const std::string &endpoint)
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
            handleError(e, "client connect");
            return false;
        }
    }

    bool ZMQClient::send(const uint8_t *data, size_t size)
    {
        if (!isConnected())
        {
            return false;
        }

        try
        {
            zmq::message_t message(data, size);
            auto result = socket_->send(message, zmq::send_flags::none);
            return result.has_value();
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "client send");
            return false;
        }
    }

    ssize_t ZMQClient::receive(uint8_t *buffer, size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
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
            return static_cast<ssize_t>(receivedSize);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "client receive");
            return -1;
        }
    }

    bool ZMQClient::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }
        return send(buffer.data(), buffer.size());
    }

    bool ZMQClient::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options
        std::vector<uint8_t> buffer(2048);
        ssize_t received = receive(buffer.data(), buffer.size());
        if (received <= 0)
        {
            return false;
        }
        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
    }

} // namespace limp
