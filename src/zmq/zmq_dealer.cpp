#include "limp/zmq/zmq_dealer.hpp"
#include <cstring>

namespace limp
{

    ZMQDealer::ZMQDealer(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::dealer);
    }

    TransportError ZMQDealer::setIdentity(const std::string &identity)
    {
        if (connected_)
        {
            handleError(zmq::error_t(), "cannot set identity after connection");
            return TransportError::AlreadyConnected;
        }

        if (!socket_)
        {
            return TransportError::SocketClosed;
        }

        try
        {
            socket_->set(zmq::sockopt::routing_id, identity);
            identity_ = identity;
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer set identity");
            return TransportError::ConfigurationError;
        }
    }

    TransportError ZMQDealer::connect(const std::string &endpoint)
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
            handleError(e, "dealer connect");
            return TransportError::ConnectionFailed;
        }
    }

    TransportError ZMQDealer::sendRaw(const uint8_t *data, size_t size)
    {
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t delimiterMsg;
            auto delimiterResult = socket_->send(delimiterMsg, zmq::send_flags::sndmore);
            if (!delimiterResult)
            {
                return TransportError::SendFailed;
            }

            zmq::message_t dataMsg(data, size);
            auto result = socket_->send(dataMsg, zmq::send_flags::none);
            return result.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer send");
            return TransportError::SendFailed;
        }
    }

    std::ptrdiff_t ZMQDealer::receiveRaw(uint8_t *buffer, size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
            std::vector<zmq::message_t> parts;
            while (true)
            {
                zmq::message_t msg;
                auto result = socket_->recv(msg, zmq::recv_flags::none);

                if (!result)
                {
                    return 0;
                }

                parts.push_back(std::move(msg));

                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            if (parts.size() != 2)
            {
                handleError(zmq::error_t(), "dealer receive: expected 2 parts, got " + std::to_string(parts.size()));
                return -1;
            }

            const auto &dataMsg = parts[1];
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
            handleError(e, "dealer receive");
            return -1;
        }
    }

    std::ptrdiff_t ZMQDealer::receiveRaw(std::string &sourceIdentity,
                                          uint8_t *buffer,
                                          size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
            std::vector<zmq::message_t> parts;
            while (true)
            {
                zmq::message_t msg;
                auto result = socket_->recv(msg, zmq::recv_flags::none);

                if (!result)
                {
                    return 0; // Timeout
                }

                parts.push_back(std::move(msg));

                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            if (parts.size() != 3)
            {
                handleError(zmq::error_t(), "dealer receiveRaw with identity: expected 3 parts, got " + std::to_string(parts.size()));
                return -1;
            }

            // Extract source identity from first part
            const auto &identityMsg = parts[0];
            sourceIdentity = std::string(static_cast<const char*>(identityMsg.data()), identityMsg.size());

            // Data is in the third part (after delimiter)
            const auto &dataMsg = parts[2];
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
            handleError(e, "dealer receiveRaw with identity");
            return -1;
        }
    }

    TransportError ZMQDealer::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return TransportError::SerializationFailed;
        }
        
        return sendRaw(buffer.data(), buffer.size());
    }

    TransportError ZMQDealer::sendRaw(const std::string &destinationIdentity,
                            const uint8_t *data,
                            size_t size)
    {
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t destMsg(destinationIdentity.data(), destinationIdentity.size());
            zmq::message_t emptyMsg;
            zmq::message_t dataMsg(data, size);

            if (!socket_->send(destMsg, zmq::send_flags::sndmore))
            {
                return TransportError::SendFailed;
            }
            if (!socket_->send(emptyMsg, zmq::send_flags::sndmore))
            {
                return TransportError::SendFailed;
            }
            auto result = socket_->send(dataMsg, zmq::send_flags::none);
            return result.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer sendRaw");
            return TransportError::SendFailed;
        }
    }

    TransportError ZMQDealer::send(const std::string &destinationIdentity, const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return TransportError::SerializationFailed;
        }
        
        return sendRaw(destinationIdentity, buffer.data(), buffer.size());
    }

    TransportError ZMQDealer::receive(Frame &frame, int timeoutMs)
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

    TransportError ZMQDealer::receive(std::string &sourceIdentity, Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> buffer(4096); // Initial buffer size
        std::ptrdiff_t received = receiveRaw(sourceIdentity, buffer.data(), buffer.size());
        
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
