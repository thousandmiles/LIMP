#include "limp/zmq/zmq_publisher.hpp"

namespace limp
{

    ZMQPublisher::ZMQPublisher(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::pub);
    }

    bool ZMQPublisher::bind(const std::string &endpoint)
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
            handleError(e, "publisher bind");
            return false;
        }
    }

    bool ZMQPublisher::send(const uint8_t *data, size_t size)
    {
        return publish("", data, size);
    }

    bool ZMQPublisher::publish(const std::string &topic, const uint8_t *data, size_t size)
    {
        if (!isConnected())
        {
            return false;
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
                    return false;
                }
            }

            // Send data as second part
            zmq::message_t dataMsg(data, size);
            auto result = socket_->send(dataMsg, zmq::send_flags::none);
            return result.has_value();
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "publisher send");
            return false;
        }
    }

    bool ZMQPublisher::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }
        return send(buffer.data(), buffer.size());
    }

    bool ZMQPublisher::receive(Frame &frame, int timeoutMs)
    {
        // Publishers don't receive
        (void)frame;
        (void)timeoutMs;
        return false;
    }

    std::ptrdiff_t ZMQPublisher::receive(uint8_t *buffer, size_t maxSize)
    {
        // Publishers don't receive
        (void)buffer;
        (void)maxSize;
        return -1;
    }

} // namespace limp
