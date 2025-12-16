#include "limp/zmq/zmq_dealer.hpp"
#include <cstring>

namespace limp
{

    ZMQDealer::ZMQDealer(const ZMQConfig &config)
        : ZMQTransport(config)
    {
        createSocket(zmq::socket_type::dealer);
    }

    bool ZMQDealer::setIdentity(const std::string &identity)
    {
        if (connected_)
        {
            handleError(zmq::error_t(), "cannot set identity after connection");
            return false;
        }

        if (!socket_)
        {
            return false;
        }

        try
        {
            socket_->set(zmq::sockopt::routing_id, identity);
            identity_ = identity;
            return true;
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer set identity");
            return false;
        }
    }

    bool ZMQDealer::connect(const std::string &endpoint)
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
            handleError(e, "dealer connect");
            return false;
        }
    }

    bool ZMQDealer::sendRaw(const uint8_t *data, size_t size)
    {
        if (!isConnected())
        {
            return false;
        }

        try
        {
            // For direct ROUTER-DEALER communication: send [empty_delimiter][data]
            // Send empty delimiter first
            zmq::message_t delimiterMsg;
            auto delimiterResult = socket_->send(delimiterMsg, zmq::send_flags::sndmore);
            if (!delimiterResult)
            {
                return false;
            }

            // Send data
            zmq::message_t dataMsg(data, size);
            auto result = socket_->send(dataMsg, zmq::send_flags::none);
            return result.has_value();
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer send");
            return false;
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
            // DEALER receives multi-part messages from ROUTER
            // Format after ROUTER strips our identity: [delimiter][data]
            // We need to skip the delimiter and read the data part

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

            // If single-part message, it's the data directly
            // If multi-part message, last part is the data (skip delimiter)
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
            handleError(e, "dealer receive");
            return -1;
        }
    }

    bool ZMQDealer::send(const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }
        return sendRaw(buffer.data(), buffer.size());
    }

    bool ZMQDealer::sendRaw(const std::string &destinationIdentity,
                            const uint8_t *data,
                            size_t size)
    {
        if (!isConnected())
        {
            return false;
        }

        try
        {
            // Send multipart message: [destination_identity][empty delimiter][data]
            // The empty delimiter is required by ZeroMQ ROUTER envelope protocol
            zmq::message_t destMsg(destinationIdentity.data(), destinationIdentity.size());
            zmq::message_t emptyMsg;
            zmq::message_t dataMsg(data, size);

            if (!socket_->send(destMsg, zmq::send_flags::sndmore))
            {
                return false;
            }
            if (!socket_->send(emptyMsg, zmq::send_flags::sndmore))
            {
                return false;
            }
            auto result = socket_->send(dataMsg, zmq::send_flags::none);
            return result.has_value();
        }
        catch (const zmq::error_t &e)
        {
            handleError(e, "dealer sendRaw");
            return false;
        }
    }

    bool ZMQDealer::send(const std::string &destinationIdentity, const Frame &frame)
    {
        std::vector<uint8_t> buffer;
        if (!serializeFrame(frame, buffer))
        {
            return false;
        }
        return sendRaw(destinationIdentity, buffer.data(), buffer.size());
    }

    bool ZMQDealer::receive(Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> buffer(4096); // Reasonable default size
        std::ptrdiff_t received = receiveRaw(buffer.data(), buffer.size());

        if (received <= 0)
        {
            return false;
        }

        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
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

                // Check if more parts are coming
                auto more = socket_->get(zmq::sockopt::rcvmore);
                if (!more)
                {
                    break;
                }
            }

            // Dealer receives: [identity][delimiter][data]
            // We need at least 3 parts
            if (parts.size() < 3)
            {
                handleError(zmq::error_t(), "dealer receive: incomplete message");
                return -1;
            }

            // Extract identity (first part)
            const auto &identityMsg = parts[0];
            sourceIdentity.assign(static_cast<const uint8_t *>(identityMsg.data()),
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
            handleError(e, "dealer receiveRaw");
            return -1;
        }
    }

    bool ZMQDealer::receive(std::string &sourceIdentity, Frame &frame, int timeoutMs)
    {
        (void)timeoutMs; // Timeout is set via socket options

        std::vector<uint8_t> buffer(4096);
        std::ptrdiff_t received = receiveRaw(sourceIdentity, buffer.data(), buffer.size());

        if (received <= 0)
        {
            return false;
        }

        buffer.resize(static_cast<size_t>(received));
        return deserializeFrame(buffer, frame);
    }

} // namespace limp
