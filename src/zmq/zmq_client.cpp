#include "limp/zmq/zmq_client.hpp"
#include <cstring>

namespace limp
{

    ZMQClient::ZMQClient(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::req);
    }

    TransportError ZMQClient::connect(const std::string &endpoint)
    {
        if (!socket_)
        {
            return TransportError::SocketClosed;
        }

        try
        {
            socket_->connect(endpoint);
            endpoint_ = endpoint;
            connected_ = true;
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "client connect");
            return TransportError::ConnectionFailed;
        }
    }

    TransportError ZMQClient::send(const uint8_t *data, size_t size)
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
            handleError(e, "client send");
            return TransportError::SendFailed;
        }
    }

    std::ptrdiff_t ZMQClient::receive(uint8_t *buffer, size_t maxSize)
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
            handleError(e, "client receive");
            return -1;
        }
    }

    TransportError ZMQClient::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return TransportError::SerializationFailed;
        }
        
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t message(buffer.data(), buffer.size());
            auto result = socket_->send(message, zmq::send_flags::none);
            return result.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "client send");
            return TransportError::SendFailed;
        }
    }

    TransportError ZMQClient::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options
        
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t message;
            auto result = socket_->recv(message, zmq::recv_flags::none);

            if (!result)
            {
                return TransportError::Timeout;
            }

            std::vector<uint8_t> buffer(static_cast<const uint8_t*>(message.data()),
                                       static_cast<const uint8_t*>(message.data()) + message.size());
            
            if (!deserializeFrame(buffer, frame))
            {
                return TransportError::DeserializationFailed;
            }
            
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "client receive");
            return TransportError::ReceiveFailed;
        }
    }

} // namespace limp
