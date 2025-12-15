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
            // Receive source identity frame (ROUTER adds this automatically)
            zmq::message_t srcIdentityMsg;
            auto result1 = socket_->recv(srcIdentityMsg, zmq::recv_flags::none);
            if (!result1 || !srcIdentityMsg.more())
            {
                return 0; // Timeout or incomplete message
            }
            sourceIdentity.assign(static_cast<uint8_t *>(srcIdentityMsg.data()),
                                 static_cast<uint8_t *>(srcIdentityMsg.data()) + srcIdentityMsg.size());

            // Receive first delimiter
            zmq::message_t delimiter1Msg;
            auto result2 = socket_->recv(delimiter1Msg, zmq::recv_flags::none);
            if (!result2 || !delimiter1Msg.more())
            {
                handleError(zmq::error_t(), "router receive: missing first delimiter");
                return -1;
            }

            // Receive destination identity
            zmq::message_t destIdentityMsg;
            auto result3 = socket_->recv(destIdentityMsg, zmq::recv_flags::none);
            if (!result3 || !destIdentityMsg.more())
            {
                handleError(zmq::error_t(), "router receive: missing destination identity");
                return -1;
            }
            destinationIdentity.assign(static_cast<uint8_t *>(destIdentityMsg.data()),
                                      static_cast<uint8_t *>(destIdentityMsg.data()) + destIdentityMsg.size());

            // Receive second delimiter
            zmq::message_t delimiter2Msg;
            auto result4 = socket_->recv(delimiter2Msg, zmq::recv_flags::none);
            if (!result4 || !delimiter2Msg.more())
            {
                handleError(zmq::error_t(), "router receive: missing second delimiter");
                return -1;
            }

            // Receive data frame
            zmq::message_t dataMsg;
            auto result5 = socket_->recv(dataMsg, zmq::recv_flags::none);
            if (!result5)
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
