#include "limp/zmq/zmq_subscriber.hpp"
#include <cstring>
#include <vector>

namespace limp
{

    ZMQSubscriber::ZMQSubscriber(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::sub);
    }

    TransportError ZMQSubscriber::connect(const std::string &endpoint)
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
            handleError(e, "subscriber connect");
            return TransportError::ConnectionFailed;
        }
    }

    TransportError ZMQSubscriber::subscribe(const std::string &topic)
    {
        if (!socket_)
        {
            return TransportError::SocketClosed;
        }

        try
        {
            socket_->set(zmq::sockopt::subscribe, topic);
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "subscriber subscribe");
            return TransportError::ConfigurationError;
        }
    }

    TransportError ZMQSubscriber::unsubscribe(const std::string &topic)
    {
        if (!socket_)
        {
            return TransportError::SocketClosed;
        }

        try
        {
            socket_->set(zmq::sockopt::unsubscribe, topic);
            return TransportError::None;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "subscriber unsubscribe");
            return TransportError::ConfigurationError;
        }
    }

    TransportError ZMQSubscriber::send(const Frame &frame)
    {
        // Subscribers don't send
        (void)frame;
        return TransportError::InternalError;
    }

    TransportError ZMQSubscriber::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options
        
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            // Receive all parts of the message
            std::vector<zmq::message_t> messages;

            while (true)
            {
                zmq::message_t message;
                auto result = socket_->recv(message, zmq::recv_flags::none);

                if (!result)
                {
                    return TransportError::Timeout;
                }

                messages.push_back(std::move(message));

                // Check if more parts are coming
                auto more = socket_->get(zmq::sockopt::rcvmore);

                if (!more)
                {
                    break;
                }
            }

            // If there are multiple parts, the first part is the topic (skip it)
            // The actual frame data is in the last part
            size_t dataPartIndex = (messages.size() > 1) ? messages.size() - 1 : 0;
            
            const auto &dataMsg = messages[dataPartIndex];
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
            handleError(e, "subscriber receive");
            return TransportError::ReceiveFailed;
        }
    }

    std::ptrdiff_t ZMQSubscriber::receive(uint8_t *buffer, size_t maxSize)
    {
        if (!isConnected())
        {
            return -1;
        }

        try
        {
            // Receive all parts of the message
            std::vector<zmq::message_t> messages;

            while (true)
            {
                zmq::message_t message;
                auto result = socket_->recv(message, zmq::recv_flags::none);

                if (!result)
                {
                    return 0; // Timeout
                }

                messages.push_back(std::move(message));

                // Check if more parts are coming
                auto more = socket_->get(zmq::sockopt::rcvmore);

                if (!more)
                {
                    break;
                }
            }

            // If there are multiple parts, the first part is the topic (skip it)
            // The actual frame data is in the last part
            size_t dataPartIndex = (messages.size() > 1) ? messages.size() - 1 : 0;
            
            const auto &dataMsg = messages[dataPartIndex];
            size_t dataSize = dataMsg.size();

            if (dataSize > maxSize)
            {
                handleError(zmq::error_t(), "received message larger than buffer");
                return -1;
            }

            // Copy only the data part to buffer (skip topic if present)
            std::memcpy(buffer, dataMsg.data(), dataSize);

            return static_cast<std::ptrdiff_t>(dataSize);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "subscriber receive");
            return -1;
        }
    }

} // namespace limp
