#pragma once

#include <cstdint>
#include <string>

namespace limp
{

    /** @brief LIMP protocol version number */
    constexpr uint8_t PROTOCOL_VERSION = 0x01;

    /** @brief Minimum valid frame size (header only) */
    constexpr uint16_t MIN_FRAME_SIZE = 16;

    /** @brief Maximum payload size in bytes */
    constexpr uint16_t MAX_PAYLOAD_SIZE = 65534;

    /** @brief Fixed header size in bytes */
    constexpr uint16_t HEADER_SIZE = 16;

    /** @brief CRC16 checksum size in bytes */
    constexpr uint16_t CRC_SIZE = 2;

    /**
     * @brief Message type identifiers
     *
     * Defines the purpose and semantics of a LIMP message.
     */
    enum class MsgType : uint8_t
    {
        REQUEST = 0x01,     ///< Request data or action from target
        RESPONSE = 0x02,    ///< Response to a REQUEST
        EVENT = 0x03,       ///< Unsolicited event notification
        ERROR = 0x04,       ///< Error response
        SUBSCRIBE = 0x05,   ///< Subscribe to data changes
        UNSUBSCRIBE = 0x06, ///< Unsubscribe from data changes
        ACK = 0x07          ///< Acknowledgment
    };

    /**
     * @brief Error code identifiers
     *
     * Standard error codes for ERROR message types.
     */
    enum class ErrorCode : uint8_t
    {
        InvalidClass = 0x01,       ///< Class ID not recognized
        InvalidInstance = 0x02,    ///< Instance ID not found
        InvalidAttribute = 0x03,   ///< Attribute ID not supported
        PermissionDenied = 0x04,   ///< Access denied
        BadPayload = 0x05,         ///< Invalid payload data
        InternalError = 0x06,      ///< Internal server error
        UnsupportedVersion = 0x07, ///< Protocol version not supported
        InvalidFlags = 0x08        ///< Invalid frame flags
    };

    /**
     * @brief Payload data type identifiers
     *
     * Defines the type and encoding of payload data.
     */
    enum class PayloadType : uint8_t
    {
        NONE = 0x00,    ///< No payload
        UINT8 = 0x01,   ///< 8-bit unsigned integer
        UINT16 = 0x02,  ///< 16-bit unsigned integer (big-endian)
        UINT32 = 0x03,  ///< 32-bit unsigned integer (big-endian)
        UINT64 = 0x04,  ///< 64-bit unsigned integer (big-endian)
        FLOAT32 = 0x05, ///< 32-bit IEEE 754 float (big-endian)
        FLOAT64 = 0x06, ///< 64-bit IEEE 754 double (big-endian)
        STRING = 0x07,  ///< UTF-8 string (length-prefixed)
        OPAQUE = 0x08   ///< Opaque binary data
    };

    /**
     * @brief Quality values for Tag.Quality attribute
     *
     * Indicates data quality status for tag values.
     */
    enum class Quality : uint8_t
    {
        Bad = 0,      ///< Data is not reliable
        Good = 1,     ///< Data is valid and reliable
        Uncertain = 2 ///< Data quality is uncertain
    };

    /**
     * @brief Alarm severity levels
     *
     * Standard severity classification for alarm objects.
     */
    enum class Severity : uint8_t
    {
        Info = 0,    ///< Informational message
        Warning = 1, ///< Warning condition
        Critical = 2 ///< Critical alarm
    };

    /**
     * @brief Frame flag bit definitions
     */
    namespace Flags
    {
        /** @brief Bit 0: CRC16 checksum is present at end of frame */
        constexpr uint8_t CRC_PRESENT = 0x01;

        /** @brief Bits 1-7: Reserved for future use */
        constexpr uint8_t RESERVED_MASK = 0xFE;
    }

    /**
     * @brief Get fixed size of payload type in bytes
     *
     * Returns 0 for variable-length types (STRING, OPAQUE, NONE).
     *
     * @param type Payload type
     * @return Size in bytes, or 0 for variable-length
     */
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

    /**
     * @name Enum to String Converters
     * Convert enum values to human-readable strings for debugging and logging.
     * @{
     */

    /** @brief Convert MsgType to string */
    const char *toString(MsgType type);

    /** @brief Convert PayloadType to string */
    const char *toString(PayloadType type);

    /** @brief Convert ErrorCode to string */
    const char *toString(ErrorCode code);

    /** @brief Convert Quality to string */
    const char *toString(Quality quality);

    /** @brief Convert Severity to string */
    const char *toString(Severity severity);

    /** @} */

} // namespace limp
