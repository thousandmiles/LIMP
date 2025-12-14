#pragma once

#include "types.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace limp
{

    /**
     * @brief LIMP Frame Structure
     *
     * Represents a complete LIMP protocol frame with 16-byte header and variable payload.
     * Binary format:
     * - Byte 0: Version (0x01)
     * - Byte 1: Message Type
     * - Bytes 2-3: Source Node ID (big-endian)
     * - Bytes 4-5: Destination Node ID (big-endian)
     * - Bytes 6-7: Class ID (big-endian)
     * - Bytes 8-9: Instance ID (big-endian)
     * - Bytes 10-11: Attribute ID (big-endian)
     * - Byte 12: Payload Type
     * - Byte 13: Flags
     * - Bytes 14-15: Payload Length (big-endian)
     * - Bytes 16+: Payload data (0-65534 bytes)
     * - Optional: CRC16-MODBUS (2 bytes) if CRC flag set
     */
    struct Frame
    {
        /** @brief Protocol version (default: 0x01) */
        uint8_t version;

        /** @brief Message type (REQUEST, RESPONSE, EVENT, etc.) */
        MsgType msgType;

        /** @brief Source node identifier */
        uint16_t srcNodeID;

        /** @brief Destination node identifier */
        uint16_t dstNodeID;

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

        /**
         * @brief Calculate total frame size in bytes
         * @return Size = HEADER_SIZE + payload.size() + (CRC_SIZE if CRC enabled)
         */
        size_t totalSize() const;

        /**
         * @brief Check if CRC validation is enabled
         * @return true if CRC_PRESENT flag is set
         */
        bool hasCRC() const { return (flags & Flags::CRC_PRESENT) != 0; }

        /**
         * @brief Enable or disable CRC16-MODBUS validation
         * @param enabled true to enable CRC, false to disable
         */
        void setCRCEnabled(bool enabled);

        /**
         * @brief Validate frame structure and constraints
         * @return true if frame is valid (version, payload size, CRC if present)
         */
        bool validate() const;
    };

    /**
     * @brief Serialize a frame to binary format
     *
     * Encodes the frame into wire format (big-endian).
     * If CRC is enabled, calculates and appends CRC16-MODBUS.
     *
     * @param frame Frame to serialize
     * @param buffer Output buffer (resized to exact frame size)
     * @return true on success, false if frame invalid
     */
    bool serializeFrame(const Frame &frame, std::vector<uint8_t> &buffer);

    /**
     * @brief Deserialize a frame from binary format
     *
     * Decodes wire format (big-endian) into Frame structure.
     * Validates CRC if present.
     *
     * @param buffer Input binary buffer
     * @param frame Output frame structure
     * @return true on success, false if invalid format or CRC mismatch
     */
    bool deserializeFrame(const std::vector<uint8_t> &buffer, Frame &frame);

    /**
     * @brief Deserialize a frame from raw buffer
     *
     * Decodes wire format from raw pointer.
     * Validates CRC if present.
     *
     * @param data Pointer to binary buffer
     * @param size Buffer size in bytes
     * @param frame Output frame structure
     * @return true on success, false if invalid format or CRC mismatch
     */
    *@param length Buffer length
            *@param frame Output frame
                *@ return true on success *
        /
        bool deserializeFrame(const uint8_t *data, size_t length, Frame &frame);

} // namespace limp
