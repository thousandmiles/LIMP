#pragma once

#include <cstdint>
#include <string>

namespace limp
{

    // Protocol version
    constexpr uint8_t PROTOCOL_VERSION = 0x01;

    // Frame size constraints
    constexpr uint16_t MIN_FRAME_SIZE = 16;
    constexpr uint16_t MAX_PAYLOAD_SIZE = 65534;
    constexpr uint16_t HEADER_SIZE = 16;
    constexpr uint16_t CRC_SIZE = 2;

    // Node IDs
    enum class NodeID : uint16_t
    {
        HMI = 0x0010,
        Server = 0x0020,
        PLC = 0x0030,
        Alarm = 0x0040,
        Logger = 0x0050,
        Broadcast = 0xFFFF
    };

    // Node ID ranges
    constexpr uint16_t NODE_PROTOCOL_BASE = 0x0001;
    constexpr uint16_t NODE_PROTOCOL_END = 0x6FFF;
    constexpr uint16_t NODE_VENDOR_BASE = 0x7000;
    constexpr uint16_t NODE_VENDOR_END = 0x7FFF;
    constexpr uint16_t NODE_USER_BASE = 0x8000;
    constexpr uint16_t NODE_USER_END = 0xFFFE;

    // Message Types
    enum class MsgType : uint8_t
    {
        REQUEST = 0x01,
        RESPONSE = 0x02,
        EVENT = 0x03,
        ERROR = 0x04,
        SUBSCRIBE = 0x05,
        UNSUBSCRIBE = 0x06,
        ACK = 0x07
    };

    // Class IDs
    enum class ClassID : uint16_t
    {
        System = 0x1000,
        IO = 0x2000,
        Tag = 0x3000,
        Motion = 0x4000,
        AlarmObject = 0x5000,
        LoggerObject = 0x6000
    };

    // Class ID ranges
    constexpr uint16_t CLASS_PROTOCOL_BASE = 0x1000;
    constexpr uint16_t CLASS_PROTOCOL_END = 0x6FFF;
    constexpr uint16_t CLASS_VENDOR_BASE = 0x7000;
    constexpr uint16_t CLASS_VENDOR_END = 0x7FFF;
    constexpr uint16_t CLASS_USER_BASE = 0x8000;
    constexpr uint16_t CLASS_USER_END = 0xFFFF;

    // Instance ID range
    constexpr uint16_t INSTANCE_CORE_MAX = 0x7FFF;
    constexpr uint16_t INSTANCE_USER_BASE = 0x8000;

    // Attribute IDs for Tag class
    namespace TagAttr
    {
        constexpr uint16_t Value = 0x0001;
        constexpr uint16_t Quality = 0x0002;
        constexpr uint16_t Timestamp = 0x0003;
    }

    // Attribute IDs for Motion class
    namespace MotionAttr
    {
        constexpr uint16_t Position = 0x0001;
        constexpr uint16_t Speed = 0x0002;
        constexpr uint16_t Torque = 0x0003;
    }

    // Attribute IDs for AlarmObject class
    namespace AlarmAttr
    {
        constexpr uint16_t Active = 0x0001;
        constexpr uint16_t Severity = 0x0002;
        constexpr uint16_t Message = 0x0003;
    }

    // Error Codes
    enum class ErrorCode : uint8_t
    {
        InvalidClass = 0x01,
        InvalidInstance = 0x02,
        InvalidAttribute = 0x03,
        PermissionDenied = 0x04,
        BadPayload = 0x05,
        InternalError = 0x06,
        UnsupportedVersion = 0x07,
        InvalidFlags = 0x08
    };

    // Payload Types
    enum class PayloadType : uint8_t
    {
        NONE = 0x00,
        UINT8 = 0x01,
        UINT16 = 0x02,
        UINT32 = 0x03,
        UINT64 = 0x04,
        FLOAT32 = 0x05,
        FLOAT64 = 0x06,
        STRING = 0x07,
        OPAQUE = 0x08
    };

    // Quality values for Tag.Quality attribute
    enum class Quality : uint8_t
    {
        Bad = 0,
        Good = 1,
        Uncertain = 2
    };

    // Alarm severity values
    enum class Severity : uint8_t
    {
        Info = 0,
        Warning = 1,
        Critical = 2
    };

    // Frame flags
    namespace Flags
    {
        constexpr uint8_t CRC_PRESENT = 0x01;
        constexpr uint8_t RESERVED_MASK = 0xFE;
    }

    // Get payload type size (returns 0 for variable-length types)
    inline uint16_t getPayloadTypeSize(PayloadType type)
    {
        switch (type)
        {
        case PayloadType::NONE:
            return 0;
        case PayloadType::UINT8:
            return 1;
        case PayloadType::UINT16:
            return 2;
        case PayloadType::UINT32:
            return 4;
        case PayloadType::UINT64:
            return 8;
        case PayloadType::FLOAT32:
            return 4;
        case PayloadType::FLOAT64:
            return 8;
        case PayloadType::STRING:
            return 0; // Variable
        case PayloadType::OPAQUE:
            return 0; // Variable
        default:
            return 0;
        }
    }

    // Convert enums to strings for debugging
    const char *toString(MsgType type);
    const char *toString(PayloadType type);
    const char *toString(ErrorCode code);
    const char *toString(Quality quality);
    const char *toString(Severity severity);

} // namespace limp
