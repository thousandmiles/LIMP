#include "limp/zmq/zmq_router.hpp"
#include <cstring>

namespace limp
{

    ZMQRouter::ZMQRouter(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::router);
    }

    TransportError ZMQRouter::bind(const std::string &endpoint)
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
            handleError(e, "router bind");
            return TransportError::BindFailed;
        }
    }

    std::ptrdiff_t ZMQRouter::receiveRaw(std::vector<uint8_t> &identity,
                                         uint8_t *buffer,
                                         size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
            // Receive all message parts
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

            if (parts.size() != 3)
            {
                handleError(zmq::error_t(), "router receive: expected 3 parts, got " + std::to_string(parts.size()));
                return -1;
            }

            const auto &identityMsg = parts[0];
            identity.assign(static_cast<const uint8_t *>(identityMsg.data()),
                            static_cast<const uint8_t *>(identityMsg.data()) + identityMsg.size());

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
            handleError(e, "router receive");
            return -1;
        }
    }

    TransportError ZMQRouter::receive(std::string &sourceIdentity,
                            Frame &frame,
                            int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> identityVec;
        std::vector<uint8_t> buffer(4096); // Initial buffer size
        std::ptrdiff_t received = receiveRaw(identityVec, buffer.data(), buffer.size());
        
        if (received < 0)
        {
            return TransportError::ReceiveFailed;
        }
        if (received == 0)
        {
            return TransportError::Timeout;
        }
        
        // Convert identity vector to string
        sourceIdentity.assign(identityVec.begin(), identityVec.end());
        buffer.resize(static_cast<size_t>(received));
        
        if (!deserializeFrame(buffer, frame))
        {
            return TransportError::DeserializationFailed;
        }
        
        return TransportError::None;
    }

    std::ptrdiff_t ZMQRouter::receiveRaw(std::vector<uint8_t> &sourceIdentity,
                                         std::vector<uint8_t> &destinationIdentity,
                                         uint8_t *buffer,
                                         size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
            // Receive all message parts
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

                // Check if more parts are coming
                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            if (parts.size() != 4)
            {
                handleError(zmq::error_t(), "router receive: expected 4 parts, got " + std::to_string(parts.size()));
                return -1;
            }

            const auto &srcIdentityMsg = parts[0];
            sourceIdentity.assign(static_cast<const uint8_t *>(srcIdentityMsg.data()),
                                  static_cast<const uint8_t *>(srcIdentityMsg.data()) + srcIdentityMsg.size());

            const auto &destIdentityMsg = parts[1];
            destinationIdentity.assign(static_cast<const uint8_t *>(destIdentityMsg.data()),
                                       static_cast<const uint8_t *>(destIdentityMsg.data()) + destIdentityMsg.size());

            const auto &dataMsg = parts[3];
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

    TransportError ZMQRouter::receive(std::string &sourceIdentity,
                            std::string &destinationIdentity,
                            Frame &frame,
                            int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> sourceIdentityVec;
        std::vector<uint8_t> destinationIdentityVec;
        std::vector<uint8_t> buffer(4096); // Initial buffer size
        std::ptrdiff_t received = receiveRaw(sourceIdentityVec, destinationIdentityVec, buffer.data(), buffer.size());
        
        if (received < 0)
        {
            return TransportError::ReceiveFailed;
        }
        if (received == 0)
        {
            return TransportError::Timeout;
        }
        
        // Convert identity vectors to strings
        sourceIdentity.assign(sourceIdentityVec.begin(), sourceIdentityVec.end());
        destinationIdentity.assign(destinationIdentityVec.begin(), destinationIdentityVec.end());
        buffer.resize(static_cast<size_t>(received));
        
        if (!deserializeFrame(buffer, frame))
        {
            return TransportError::DeserializationFailed;
        }
        
        return TransportError::None;
    }

    TransportError ZMQRouter::sendRaw(const std::vector<uint8_t> &identity,
                            const uint8_t *data,
                            size_t size)
    {
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t identityMsg(identity.data(), identity.size());
            auto identityResult = socket_->send(identityMsg, zmq::send_flags::sndmore);
            if (!identityResult)
            {
                return TransportError::SendFailed;
            }

            zmq::message_t delimiterMsg;
            auto delimiterResult = socket_->send(delimiterMsg, zmq::send_flags::sndmore);
            if (!delimiterResult)
            {
                return TransportError::SendFailed;
            }

            zmq::message_t dataMsg(data, size);
            auto dataResult = socket_->send(dataMsg, zmq::send_flags::none);
            return dataResult.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router send");
            return TransportError::SendFailed;
        }
    }

    TransportError ZMQRouter::sendRaw(const std::vector<uint8_t> &clientIdentity,
                            const std::vector<uint8_t> &sourceIdentity,
                            const uint8_t *data,
                            size_t size)
    {
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t clientIdentityMsg(clientIdentity.data(), clientIdentity.size());
            auto clientIdentityResult = socket_->send(clientIdentityMsg, zmq::send_flags::sndmore);
            if (!clientIdentityResult)
            {
                return TransportError::SendFailed;
            }

            zmq::message_t sourceIdentityMsg(sourceIdentity.data(), sourceIdentity.size());
            auto sourceIdentityResult = socket_->send(sourceIdentityMsg, zmq::send_flags::sndmore);
            if (!sourceIdentityResult)
            {
                return TransportError::SendFailed;
            }

            zmq::message_t delimiterMsg;
            auto delimiterResult = socket_->send(delimiterMsg, zmq::send_flags::sndmore);
            if (!delimiterResult)
            {
                return TransportError::SendFailed;
            }

            zmq::message_t dataMsg(data, size);
            auto dataResult = socket_->send(dataMsg, zmq::send_flags::none);
            return dataResult.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router send");
            return TransportError::SendFailed;
        }
    }

    TransportError ZMQRouter::send(const std::string &clientIdentity, const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return TransportError::SerializationFailed;
        }

        std::vector<uint8_t> identityVec(clientIdentity.begin(), clientIdentity.end());
        return sendRaw(identityVec, buffer.data(), buffer.size());
    }

    TransportError ZMQRouter::send(const std::string &clientIdentity,
                         const std::string &sourceIdentity,
                         const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return TransportError::SerializationFailed;
        }

        std::vector<uint8_t> clientIdentityVec(clientIdentity.begin(), clientIdentity.end());
        std::vector<uint8_t> sourceIdentityVec(sourceIdentity.begin(), sourceIdentity.end());
        return sendRaw(clientIdentityVec, sourceIdentityVec, buffer.data(), buffer.size());
    }

    TransportError ZMQRouter::send(const Frame &frame)
    {
        (void)frame;
        handleError(zmq::error_t(), "router requires client identity for send");
        return TransportError::InternalError;
    }

    TransportError ZMQRouter::sendRaw(const uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        handleError(zmq::error_t(), "router requires client identity for send");
        return TransportError::InternalError;
    }

    TransportError ZMQRouter::receive(Frame &frame, int timeoutMs)
    {
        (void)frame;
        (void)timeoutMs;
        handleError(zmq::error_t(), "router requires identity output for receive");
        return TransportError::InternalError;
    }

    std::ptrdiff_t ZMQRouter::receiveRaw(uint8_t *buffer, size_t maxSize)
    {
        (void)buffer;
        (void)maxSize;
        handleError(zmq::error_t(), "router requires identity output for receive");
        return -1;
    }

} // namespace limp
