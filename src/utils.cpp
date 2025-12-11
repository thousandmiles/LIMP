#include "limp/types.hpp"

namespace limp
{

    const char *toString(MsgType type)
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

    const char *toString(PayloadType type)
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

    const char *toString(ErrorCode code)
    {
        switch (code)
        {
        case ErrorCode::InvalidClass:
            return "InvalidClass";
        case ErrorCode::InvalidInstance:
            return "InvalidInstance";
        case ErrorCode::InvalidAttribute:
            return "InvalidAttribute";
        case ErrorCode::PermissionDenied:
            return "PermissionDenied";
        case ErrorCode::BadPayload:
            return "BadPayload";
        case ErrorCode::InternalError:
            return "InternalError";
        case ErrorCode::UnsupportedVersion:
            return "UnsupportedVersion";
        case ErrorCode::InvalidFlags:
            return "InvalidFlags";
        default:
            return "UNKNOWN";
        }
    }

    const char *toString(Quality quality)
    {
        switch (quality)
        {
        case Quality::Bad:
            return "Bad";
        case Quality::Good:
            return "Good";
        case Quality::Uncertain:
            return "Uncertain";
        default:
            return "UNKNOWN";
        }
    }

    const char *toString(Severity severity)
    {
        switch (severity)
        {
        case Severity::Info:
            return "Info";
        case Severity::Warning:
            return "Warning";
        case Severity::Critical:
            return "Critical";
        default:
            return "UNKNOWN";
        }
    }

} // namespace limp
