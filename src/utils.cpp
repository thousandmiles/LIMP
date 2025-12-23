#include "limp/types.hpp"
#include "limp/transport.hpp"

namespace limp
{

    const char *toString(MsgType type) noexcept
    {
        switch (type)
        {
        case MsgType::REQUEST:
            return "REQUEST";
        case MsgType::RESPONSE:
            return "RESPONSE";
        case MsgType::EVENT:
            return "EVENT";
        case MsgType::ERROR:
            return "ERROR";
        case MsgType::SUBSCRIBE:
            return "SUBSCRIBE";
        case MsgType::UNSUBSCRIBE:
            return "UNSUBSCRIBE";
        case MsgType::ACK:
            return "ACK";
        default:
            return "UNKNOWN";
        }
    }

    const char *toString(PayloadType type) noexcept
    {
        switch (type)
        {
        case PayloadType::NONE:
            return "NONE";
        case PayloadType::UINT8:
            return "UINT8";
        case PayloadType::UINT16:
            return "UINT16";
        case PayloadType::UINT32:
            return "UINT32";
        case PayloadType::UINT64:
            return "UINT64";
        case PayloadType::FLOAT32:
            return "FLOAT32";
        case PayloadType::FLOAT64:
            return "FLOAT64";
        case PayloadType::STRING:
            return "STRING";
        case PayloadType::OPAQUE:
            return "OPAQUE";
        default:
            return "UNKNOWN";
        }
    }

    const char *toString(TransportError error) noexcept
    {
        switch (error)
        {
        case TransportError::None:
            return "None";
        case TransportError::ConnectionFailed:
            return "ConnectionFailed";
        case TransportError::BindFailed:
            return "BindFailed";
        case TransportError::SendFailed:
            return "SendFailed";
        case TransportError::ReceiveFailed:
            return "ReceiveFailed";
        case TransportError::Timeout:
            return "Timeout";
        case TransportError::InvalidEndpoint:
            return "InvalidEndpoint";
        case TransportError::SocketClosed:
            return "SocketClosed";
        case TransportError::NotConnected:
            return "NotConnected";
        case TransportError::SerializationFailed:
            return "SerializationFailed";
        case TransportError::DeserializationFailed:
            return "DeserializationFailed";
        case TransportError::InvalidFrame:
            return "InvalidFrame";
        case TransportError::AlreadyConnected:
            return "AlreadyConnected";
        case TransportError::ConfigurationError:
            return "ConfigurationError";
        case TransportError::InternalError:
            return "InternalError";
        default:
            return "UNKNOWN";
        }
    }

} // namespace limp
