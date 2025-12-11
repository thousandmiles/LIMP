#pragma once

#include "types.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

namespace limp
{

    /**
     * LIMP Frame Structure
     * Represents a complete LIMP protocol frame with header and payload
     */
    struct Frame
    {
        // Header fields
        uint8_t version;
        MsgType msgType;
        uint16_t srcNodeID;
        uint16_t dstNodeID;
        uint16_t classID;
        uint16_t instanceID;
        uint16_t attrID;
        PayloadType payloadType;
        uint16_t payloadLen;
        uint8_t flags;

        // Payload data
        std::vector<uint8_t> payload;

        // CRC (if flags & CRC_PRESENT)
        std::optional<uint16_t> crc;

        Frame();

        /**
         * Calculate total frame size in bytes
         */
        size_t totalSize() const;

        /**
         * Check if CRC is enabled
         */
        bool hasCRC() const { return (flags & Flags::CRC_PRESENT) != 0; }

        /**
         * Enable/disable CRC
         */
        void setCRCEnabled(bool enabled);

        /**
         * Validate frame structure
         */
        bool validate() const;
    };

    /**
     * Serialize a frame to binary format
     *
     * @param frame Frame to serialize
     * @param buffer Output buffer (will be resized)
     * @return true on success
     */
    bool serializeFrame(const Frame &frame, std::vector<uint8_t> &buffer);

    /**
     * Deserialize a frame from binary format
     *
     * @param buffer Input buffer
     * @param frame Output frame
     * @return true on success
     */
    bool deserializeFrame(const std::vector<uint8_t> &buffer, Frame &frame);

    /**
     * Deserialize a frame from raw buffer
     *
     * @param data Pointer to buffer
     * @param length Buffer length
     * @param frame Output frame
     * @return true on success
     */
    bool deserializeFrame(const uint8_t *data, size_t length, Frame &frame);

} // namespace limp
