#include "limp/frame.hpp"
#include "limp/crc.hpp"
#include "limp/utils.hpp"
#include <cstring>

namespace limp
{

    Frame::Frame()
        : version(PROTOCOL_VERSION), msgType(MsgType::REQUEST), srcNodeID(0), classID(0), instanceID(0), attrID(0), payloadType(PayloadType::NONE), payloadLen(0), flags(0)
    {
    }

    size_t Frame::totalSize() const
    {
        size_t size = HEADER_SIZE + payloadLen;
        if (hasCRC())
        {
            size += CRC_SIZE;
        }
        return size;
    }

    void Frame::setCRCEnabled(bool enabled)
    {
        if (enabled)
        {
            flags |= Flags::CRC_PRESENT;
        }
        else
        {
            flags &= ~Flags::CRC_PRESENT;
        }
    }

    bool Frame::validate() const
    {
        // Check version
        if (version != PROTOCOL_VERSION)
        {
            return false;
        }

        // Check reserved flags
        if (flags & Flags::RESERVED_MASK)
        {
            return false;
        }

        // Validate payload length for fixed-size types
        uint16_t expectedSize = getPayloadTypeSize(payloadType);
        if (expectedSize > 0 && payloadLen != expectedSize)
        {
            return false;
        }

        // Check payload length limits
        if (payloadLen > MAX_PAYLOAD_SIZE)
        {
            return false;
        }

        // Verify actual payload size matches header
        if (payload.size() != payloadLen)
        {
            return false;
        }

        return true;
    }

    bool serializeFrame(const Frame &frame, std::vector<uint8_t> &buffer)
    {
        if (!frame.validate())
        {
            return false;
        }

        // Resize buffer
        buffer.resize(frame.totalSize());

        size_t offset = 0;

        // 0： Version
        buffer[offset++] = frame.version;

        // 1： MsgType
        buffer[offset++] = static_cast<uint8_t>(frame.msgType);

        // 2： SrcNodeID (big-endian)
        uint16_t srcBE = utils::hton16(frame.srcNodeID);
        std::memcpy(&buffer[offset], &srcBE, 2);
        offset += 2;

        // 4： ClassID (big-endian)
        uint16_t classBE = utils::hton16(frame.classID);
        std::memcpy(&buffer[offset], &classBE, 2);
        offset += 2;

        // 6： InstanceID (big-endian)
        uint16_t instBE = utils::hton16(frame.instanceID);
        std::memcpy(&buffer[offset], &instBE, 2);
        offset += 2;

        // 8： AttrID (big-endian)
        uint16_t attrBE = utils::hton16(frame.attrID);
        std::memcpy(&buffer[offset], &attrBE, 2);
        offset += 2;

        // 10： PayloadTypeID
        buffer[offset++] = static_cast<uint8_t>(frame.payloadType);

        // 11-12： PayloadLen (big-endian)
        uint16_t lenBE = utils::hton16(frame.payloadLen);
        std::memcpy(&buffer[offset], &lenBE, 2);
        offset += 2;

        // 13： Flags
        buffer[offset++] = frame.flags;

        // Payload
        if (frame.payloadLen > 0)
        {
            std::memcpy(&buffer[offset], frame.payload.data(), frame.payloadLen);
            offset += frame.payloadLen;
        }

        // CRC (if enabled)
        if (frame.hasCRC())
        {
            uint16_t crc = calculateCRC16(buffer.data(), offset);
            uint16_t crcBE = utils::hton16(crc);
            std::memcpy(&buffer[offset], &crcBE, 2);
        }

        return true;
    }

    bool deserializeFrame(const std::vector<uint8_t> &buffer, Frame &frame)
    {
        return deserializeFrame(buffer.data(), buffer.size(), frame);
    }

    bool deserializeFrame(const uint8_t *data, size_t length, Frame &frame)
    {
        // Minimum frame size check
        if (length < MIN_FRAME_SIZE)
        {
            return false;
        }

        size_t offset = 0;

        // Version
        frame.version = data[offset++];
        if (frame.version != PROTOCOL_VERSION)
        {
            return false;
        }

        // MsgType
        frame.msgType = static_cast<MsgType>(data[offset++]);

        // SrcNodeID (big-endian)
        uint16_t srcBE;
        std::memcpy(&srcBE, &data[offset], 2);
        frame.srcNodeID = utils::ntoh16(srcBE);
        offset += 2;

        // ClassID (big-endian)
        uint16_t classBE;
        std::memcpy(&classBE, &data[offset], 2);
        frame.classID = utils::ntoh16(classBE);
        offset += 2;

        // InstanceID (big-endian)
        uint16_t instBE;
        std::memcpy(&instBE, &data[offset], 2);
        frame.instanceID = utils::ntoh16(instBE);
        offset += 2;

        // AttrID (big-endian)
        uint16_t attrBE;
        std::memcpy(&attrBE, &data[offset], 2);
        frame.attrID = utils::ntoh16(attrBE);
        offset += 2;

        // PayloadTypeID
        frame.payloadType = static_cast<PayloadType>(data[offset++]);

        // PayloadLen (big-endian)
        uint16_t lenBE;
        std::memcpy(&lenBE, &data[offset], 2);
        frame.payloadLen = utils::ntoh16(lenBE);
        offset += 2;

        // Flags
        frame.flags = data[offset++];

        // Check for reserved flags
        if (frame.flags & Flags::RESERVED_MASK)
        {
            return false;
        }

        // Calculate expected total size
        size_t expectedSize = HEADER_SIZE + frame.payloadLen;
        if (frame.hasCRC())
        {
            expectedSize += CRC_SIZE;
        }

        if (length != expectedSize)
        {
            return false;
        }

        // Payload
        frame.payload.clear();
        if (frame.payloadLen > 0)
        {
            frame.payload.resize(frame.payloadLen);
            std::memcpy(frame.payload.data(), &data[offset], frame.payloadLen);
            offset += frame.payloadLen;
        }

        // Verify CRC if present
        if (frame.hasCRC())
        {
            if (!verifyCRC16(data, length))
            {
                return false;
            }

            // Store CRC value
            uint16_t crcBE;
            std::memcpy(&crcBE, &data[offset], 2);
            frame.crc = utils::ntoh16(crcBE);
        }
        else
        {
            frame.crc.reset();
        }

        return frame.validate();
    }

} // namespace limp
