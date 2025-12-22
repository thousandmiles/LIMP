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

        if (!isConnected())
        {
            return TransportError::NotConnected;
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
                    return TransportError::Timeout;
                }

                parts.push_back(std::move(msg));

                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            // Expect format: [identity][delimiter][data]
            if (parts.size() < 3)
            {
                return TransportError::InvalidFrame;
            }

            // Extract source identity
            const auto &identityMsg = parts[0];
            sourceIdentity = std::string(static_cast<const char*>(identityMsg.data()), identityMsg.size());

            // Data is the last part
            const auto &dataMsg = parts[parts.size() - 1];
            std::vector<uint8_t> buffer(static_cast<const uint8_t*>(dataMsg.data()),
                                       static_cast<const uint8_t*>(dataMsg.data()) + dataMsg.size());
            
            if (!deserializeFrame(buffer, frame))
            {
                return TransportError::DeserializationFailed;
            }
            
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router receive");
            return TransportError::ReceiveFailed;
        }
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

        if (!isConnected())
        {
            return TransportError::NotConnected;
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
                    return TransportError::Timeout;
                }

                parts.push_back(std::move(msg));

                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            // Expect format: [source_identity][dest_identity][delimiter][data]
            if (parts.size() < 4)
            {
                return TransportError::InvalidFrame;
            }

            // Extract source identity
            const auto &sourceMsg = parts[0];
            sourceIdentity = std::string(static_cast<const char*>(sourceMsg.data()), sourceMsg.size());

            // Extract destination identity
            const auto &destMsg = parts[1];
            destinationIdentity = std::string(static_cast<const char*>(destMsg.data()), destMsg.size());

            // Data is the last part
            const auto &dataMsg = parts[parts.size() - 1];
            std::vector<uint8_t> buffer(static_cast<const uint8_t*>(dataMsg.data()),
                                       static_cast<const uint8_t*>(dataMsg.data()) + dataMsg.size());
            
            if (!deserializeFrame(buffer, frame))
            {
                return TransportError::DeserializationFailed;
            }
            
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router receive");
            return TransportError::ReceiveFailed;
        }
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
        
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t identityMsg(identityVec.data(), identityVec.size());
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

            zmq::message_t dataMsg(buffer.data(), buffer.size());
            auto dataResult = socket_->send(dataMsg, zmq::send_flags::none);
            return dataResult.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router send");
            return TransportError::SendFailed;
        }
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

        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            zmq::message_t clientIdentityMsg(clientIdentityVec.data(), clientIdentityVec.size());
            auto clientIdentityResult = socket_->send(clientIdentityMsg, zmq::send_flags::sndmore);
            if (!clientIdentityResult)
            {
                return TransportError::SendFailed;
            }

            zmq::message_t sourceIdentityMsg(sourceIdentityVec.data(), sourceIdentityVec.size());
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

            zmq::message_t dataMsg(buffer.data(), buffer.size());
            auto dataResult = socket_->send(dataMsg, zmq::send_flags::none);
            return dataResult.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "router send");
            return TransportError::SendFailed;
        }
    }

    TransportError ZMQRouter::send(const Frame &frame)
    {
        (void)frame;
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

} // namespace limp
