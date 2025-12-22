#pragma once

#include "types.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace limp
{

    /**
     * @brief LIMP protocol frame structure
     *
     * Represents a complete LIMP frame with 14-byte header and variable payload.
     * Wire format (big-endian):
     * - Byte 0: Version (0x01)
     * - Byte 1: Message Type
     * - Bytes 2-3: Source Node ID
     * - Bytes 4-5: Class ID
     * - Bytes 6-7: Instance ID
     * - Bytes 8-9: Attribute ID
     * - Byte 10: Payload Type
     * - Byte 11: Flags
     * - Bytes 12-13: Payload Length
     * - Bytes 14+: Payload data (0-65534 bytes)
     * - Optional: CRC16-MODBUS (2 bytes appended at end if CRC flag set)
     */
    struct Frame
    {
        /** @brief Protocol version (default: 0x01) */
        uint8_t version;

        /** @brief Message type (REQUEST, RESPONSE, EVENT, etc.) */
        MsgType msgType;

        /** @brief Source node identifier */
        uint16_t srcNodeID;

        /** @brief Object class identifier */
        uint16_t classID;

        /** @brief Object instance identifier */
        uint16_t instanceID;

        /** @brief Attribute identifier */
        uint16_t attrID;

        /** @brief Payload data type */
        PayloadType payloadType;

        /** @brief Payload length in bytes */
        uint16_t payloadLen;

        /** @brief Control flags (bit 0: CRC_PRESENT) */
        uint8_t flags;

        /** @brief Payload binary data */
        std::vector<uint8_t> payload;

        /** @brief CRC16-MODBUS checksum (present if CRC flag set) */
        std::optional<uint16_t> crc;

        /** @brief Default constructor - initializes all fields to zero/defaults */
        Frame();

        // Copy operations (implicitly defined, explicitly defaulted for clarity)
        Frame(const Frame &) = default;
        Frame &operator=(const Frame &) = default;

        // Move operations (efficient transfer of payload vector)
        Frame(Frame &&) noexcept = default;
        Frame &operator=(Frame &&) noexcept = default;

        /** @brief Destructor */
        ~Frame() = default;

        /**
         * @brief Calculate total frame size in bytes
         * @return Size = HEADER_SIZE + payload.size() + (CRC_SIZE if CRC enabled)
         */
        size_t totalSize() const;

        /**
         * @brief Check if CRC validation is enabled
         * @return true if CRC_PRESENT flag is set
         */
        bool hasCRC() const noexcept { return (flags & Flags::CRC_PRESENT) != 0; }

        /**
         * @brief Enable or disable CRC16-MODBUS validation
         * @param enabled true to enable CRC, false to disable
         */
        void setCRCEnabled(bool enabled);

        /**
         * @brief Validate frame structure and constraints
         *
         * Checks version, payload size limits, and CRC if present.
         *
         * @return true if frame is valid, false otherwise
         */
        bool validate() const;
    };

    /**
     * @brief Serialize frame to wire format
     *
     * Converts frame to binary wire format (big-endian byte order).
     * Automatically calculates and appends CRC16-MODBUS if CRC flag is set.
     *
     * @param frame Frame to serialize
     * @param buffer Output buffer (automatically resized to exact frame size)
     * @return true on success, false if frame validation fails
     */
    bool serializeFrame(const Frame &frame, std::vector<uint8_t> &buffer);

    /**
     * @brief Deserialize frame from wire format
     *
     * Converts binary wire format to Frame structure.
     * Automatically validates CRC16-MODBUS if present.
     *
     * @param buffer Input binary buffer
     * @param frame Output frame structure
     * @return true on success, false if format invalid or CRC verification fails
     */
    bool deserializeFrame(const std::vector<uint8_t> &buffer, Frame &frame);

    /**
     * @brief Deserialize frame from raw buffer
     *
     * Converts binary wire format to Frame structure (raw pointer version).
     * Automatically validates CRC16-MODBUS if present.
     *
     * @param data Pointer to binary buffer
     * @param length Buffer size in bytes
     * @param frame Output frame structure
     * @return true on success, false if format invalid or CRC verification fails
     */
    bool deserializeFrame(const uint8_t *data, size_t length, Frame &frame);

} // namespace limp
