#include "limp/zmq/zmq_server.hpp"

namespace limp
{

    ZMQServer::ZMQServer(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::rep);
    }

    bool ZMQServer::bind(const std::string &endpoint)
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
            handleError(e, "server bind");
            return false;
        }
    }

    bool ZMQServer::send(const uint8_t *data, size_t size)
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
            handleError(e, "server send");
            return false;
        }
    }

    std::ptrdiff_t ZMQServer::receive(uint8_t *buffer, size_t maxSize)
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
            return static_cast<std::ptrdiff_t>(receivedSize);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "server receive");
            return -1;
        }
    }

    bool ZMQServer::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }
        return send(buffer.data(), buffer.size());
    }

    bool ZMQServer::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options
        std::vector<uint8_t> buffer(2048);
        std::ptrdiff_t received = receive(buffer.data(), buffer.size());
        if (received <= 0)
        {
            return false;
        }
        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
    }

} // namespace limp
