#include "limp/zmq/zmq_server.hpp"
#include <cstring>

namespace limp
{

    ZMQServer::ZMQServer(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::rep);
    }

    TransportError ZMQServer::bind(const std::string &endpoint)
    {
        if (!socket_)
        {
            return TransportError::SocketClosed;
        }

        try
        {
            socket_->bind(endpoint);
            endpoint_ = endpoint;
            connected_ = true;
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "server bind");
            return TransportError::BindFailed;
        }
    }

    TransportError ZMQServer::sendRaw(const uint8_t *data, size_t size)
    {
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t message(data, size);
            auto result = socket_->send(message, zmq::send_flags::none);
            return result.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "server send");
            return TransportError::SendFailed;
        }
    }

    std::ptrdiff_t ZMQServer::receiveRaw(uint8_t *buffer, size_t maxSize)
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

    TransportError ZMQServer::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return TransportError::SerializationFailed;
        }
        
        return sendRaw(buffer.data(), buffer.size());
    }

    TransportError ZMQServer::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options
        
        std::vector<uint8_t> buffer(4096); // Initial buffer size
        std::ptrdiff_t received = receiveRaw(buffer.data(), buffer.size());
        
        if (received < 0)
        {
            return TransportError::ReceiveFailed;
        }
        if (received == 0)
        {
            return TransportError::Timeout;
        }
        
        buffer.resize(static_cast<size_t>(received));
        
        if (!deserializeFrame(buffer, frame))
        {
            return TransportError::DeserializationFailed;
        }
        
        return TransportError::None;
    }

} // namespace limp
