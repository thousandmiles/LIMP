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

    TransportError ZMQPublisher::send(const uint8_t *data, size_t size)
    {
        return publish("", data, size);
    }

    TransportError ZMQPublisher::publish(const std::string &topic, const uint8_t *data, size_t size)
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

    TransportError ZMQPublisher::send(const Frame &frame)
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
            zmq::message_t dataMsg(buffer.data(), buffer.size());
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
            zmq::message_t dataMsg(buffer.data(), buffer.size());
            auto result = socket_->send(dataMsg, zmq::send_flags::none);
            return result.has_value() ? TransportError::None : TransportError::SendFailed;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "publisher publish");
            return TransportError::SendFailed;
        }
    }

    TransportError ZMQPublisher::receive(Frame &frame, int timeoutMs)
    {
        (void)frame;
        (void)timeoutMs;
        return TransportError::InternalError; // Publishers don't receive
    }

} // namespace limp
