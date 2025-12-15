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

    bool ZMQSubscriber::connect(const std::string &endpoint)
    {
        if (!socket_)
        {
            return false;
        }

        try
        {
            socket_->connect(endpoint);
            endpoint_ = endpoint;
            connected_ = true;
            return true;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "subscriber connect");
            return false;
        }
    }

    bool ZMQSubscriber::subscribe(const std::string &topic)
    {
        if (!socket_)
        {
            return false;
        }

        try
        {
            socket_->set(zmq::sockopt::subscribe, topic);
            return true;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "subscriber subscribe");
            return false;
        }
    }

    bool ZMQSubscriber::unsubscribe(const std::string &topic)
    {
        if (!socket_)
        {
            return false;
        }

        try
        {
            socket_->set(zmq::sockopt::unsubscribe, topic);
            return true;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "subscriber unsubscribe");
            return false;
        }
    }

    bool ZMQSubscriber::send(const Frame &frame)
    {
        // Subscribers don't send
        (void)frame;
        return false;
    }

    bool ZMQSubscriber::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options
        std::vector<uint8_t> buffer(2048);
        std::ptrdiff_t received = receive(buffer.data(), buffer.size());
        if (received <= 0)
        {
            return false;
        }
        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
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

            // Calculate total size
            size_t totalSize = 0;
            for (const auto &msg : messages)
            {
                totalSize += msg.size();
            }

            if (totalSize > maxSize)
            {
                handleError(zmq::error_t(), "received message larger than buffer");
                return -1;
            }

            // Copy all parts to buffer
            size_t offset = 0;
            for (const auto &msg : messages)
            {
                std::memcpy(buffer + offset, msg.data(), msg.size());
                offset += msg.size();
            }

            return static_cast<std::ptrdiff_t>(totalSize);
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "subscriber receive");
            return -1;
        }
    }

} // namespace limp
