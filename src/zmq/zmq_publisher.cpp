#include "limp/zmq/zmq_publisher.hpp"

namespace limp
{

    ZMQPublisher::ZMQPublisher(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::pub);
    }

    TransportError ZMQPublisher::bind(const std::string &endpoint)
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
            handleError(e, "publisher bind");
            return TransportError::BindFailed;
        }
    }

    TransportError ZMQPublisher::publishRaw(const std::string &topic, const uint8_t *data, size_t size)
    {
        if (!isConnected())
        {
            return TransportError::NotConnected;
        }

        try
        {
            // Send topic as first part (if not empty)
            if (!topic.empty())
            {
                zmq::message_t topicMsg(topic.data(), topic.size());
                auto result = socket_->send(topicMsg, zmq::send_flags::sndmore);
                if (!result)
                {
                    return TransportError::SendFailed;
                }
            }

            // Send data as second part
            zmq::message_t dataMsg(data, size);
            auto result = socket_->send(dataMsg, zmq::send_flags::none);
            return result.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "publisher send");
            return TransportError::SendFailed;
        }
    }

    TransportError ZMQPublisher::publish(const std::string &topic, const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return TransportError::SerializationFailed;
        }
        
        return publishRaw(topic, buffer.data(), buffer.size());
    }

    TransportError ZMQPublisher::send(const Frame &frame)
    {
        (void)frame;
        handleError(zmq::error_t(), "publisher: use publish() instead of send()");
        return TransportError::InternalError;
    }

    TransportError ZMQPublisher::sendRaw(const uint8_t *data, size_t size)
    {
        (void)data;
        (void)size;
        handleError(zmq::error_t(), "publisher: use publishRaw() instead of sendRaw()");
        return TransportError::InternalError;
    }

    std::ptrdiff_t ZMQPublisher::receiveRaw(uint8_t *buffer, size_t maxSize)
    {
        (void)buffer;
        (void)maxSize;
        handleError(zmq::error_t(), "publisher: publishers cannot receive, only publish");
        return -1; // Publishers don't receive
    }

    TransportError ZMQPublisher::receive(Frame &frame, int timeoutMs)
    {
        (void)frame;
        (void)timeoutMs;
        handleError(zmq::error_t(), "publisher: publishers cannot receive, only publish");
        return TransportError::InternalError; // Publishers don't receive
    }

} // namespace limp
