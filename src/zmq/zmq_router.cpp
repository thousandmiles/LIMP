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
                    return 0; // Timeout
                }

                parts.push_back(std::move(msg));

                // Check if more parts are coming
                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            // ROUTER receives: [identity][delimiter][data]
            // We need at least 3 parts
            if (parts.size() < 3)
            {
                handleError(zmq::error_t(), "router receive: incomplete message");
                return -1;
            }

            // Extract identity (first part)
            const auto &identityMsg = parts[0];
            identity.assign(static_cast<const uint8_t *>(identityMsg.data()),
                            static_cast<const uint8_t *>(identityMsg.data()) + identityMsg.size());

            // Skip delimiter (second part - empty frame)
            // Extract data (last part)
            const auto &dataMsg = parts.back();
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

    bool ZMQRouter::receive(std::string &sourceIdentity,
                            Frame &frame,
                            int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> identityVec;
        std::vector<uint8_t> buffer(4096);
        std::ptrdiff_t received = receiveRaw(identityVec, buffer.data(), buffer.size());

        if (received <= 0)
        {
            return false;
        }

        // Convert identity to string
        sourceIdentity = std::string(identityVec.begin(), identityVec.end());

        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
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
                    return 0; // Timeout
                }

                parts.push_back(std::move(msg));

                // Check if more parts are coming
                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            // ROUTER receives with destination routing from DEALER send(dst, frame):
            // - DEALER sends: [dest_identity][delimiter][data] (3 parts)
            // - ROUTER prepends source identity: [source_identity][dest_identity][delimiter][data] (4 parts)
            // We need at least 4 parts for routed messages
            if (parts.size() < 4)
            {
                handleError(zmq::error_t(), "router receive: incomplete routed message");
                return -1;
            }

            // Extract source identity (first part - auto-added by ROUTER)
            const auto &srcIdentityMsg = parts[0];
            sourceIdentity.assign(static_cast<const uint8_t *>(srcIdentityMsg.data()),
                                  static_cast<const uint8_t *>(srcIdentityMsg.data()) + srcIdentityMsg.size());

            // Extract destination identity (second part - from dealer's send(dst, frame))
            const auto &destIdentityMsg = parts[1];
            destinationIdentity.assign(static_cast<const uint8_t *>(destIdentityMsg.data()),
                                       static_cast<const uint8_t *>(destIdentityMsg.data()) + destIdentityMsg.size());

            // Skip delimiter (third part - empty frame)

            // Extract data (fourth/last part)
            const auto &dataMsg = parts.back();
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

    bool ZMQRouter::receive(std::string &sourceIdentity,
                            std::string &destinationIdentity,
                            Frame &frame,
                            int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> srcIdentityVec;
        std::vector<uint8_t> destIdentityVec;
        std::vector<uint8_t> buffer(4096);

        std::ptrdiff_t received = receiveRaw(srcIdentityVec, destIdentityVec, buffer.data(), buffer.size());

        if (received <= 0)
        {
            return false;
        }

        // Convert identities to strings
        sourceIdentity = std::string(srcIdentityVec.begin(), srcIdentityVec.end());
        destinationIdentity = std::string(destIdentityVec.begin(), destIdentityVec.end());

        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
    }

    bool ZMQRouter::sendRaw(const std::vector<uint8_t> &identity,
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

    bool ZMQRouter::sendRaw(const std::vector<uint8_t> &clientIdentity,
                            const std::vector<uint8_t> &sourceIdentity,
                            const uint8_t *data,
                            size_t size)
    {
        if (!isConnected())
        {
            return false;
        }

        try
        {
            // Send client identity frame
            zmq::message_t clientIdentityMsg(clientIdentity.data(), clientIdentity.size());
            auto clientIdentityResult = socket_->send(clientIdentityMsg, zmq::send_flags::sndmore);
            if (!clientIdentityResult)
            {
                return false;
            }

            // Send source identity frame
            zmq::message_t sourceIdentityMsg(sourceIdentity.data(), sourceIdentity.size());
            auto sourceIdentityResult = socket_->send(sourceIdentityMsg, zmq::send_flags::sndmore);
            if (!sourceIdentityResult)
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

    bool ZMQRouter::send(const std::string &clientIdentity, const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }

        // Convert string identity to vector<uint8_t>
        std::vector<uint8_t> identityVec(clientIdentity.begin(), clientIdentity.end());
        return sendRaw(identityVec, buffer.data(), buffer.size());
    }

    bool ZMQRouter::send(const std::string &clientIdentity,
                         const std::string &sourceIdentity,
                         const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }

        // Convert string identities to vector<uint8_t>
        std::vector<uint8_t> clientIdentityVec(clientIdentity.begin(), clientIdentity.end());
        std::vector<uint8_t> sourceIdentityVec(sourceIdentity.begin(), sourceIdentity.end());

        return sendRaw(clientIdentityVec, sourceIdentityVec, buffer.data(), buffer.size());
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
