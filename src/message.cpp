#include "limp/message.hpp"
#include "limp/utils.hpp"
#include <cstring>

namespace limp
{

    // MessageBuilder Implementation

    MessageBuilder::MessageBuilder()
    {
        frame_.version = PROTOCOL_VERSION;
        frame_.msgType = MsgType::REQUEST;
    }

    MessageBuilder &MessageBuilder::setVersion(uint8_t version)
    {
        frame_.version = version;
        return *this;
    }

    MessageBuilder &MessageBuilder::setMsgType(MsgType type)
    {
        frame_.msgType = type;
        return *this;
    }

    MessageBuilder &MessageBuilder::setSrcNode(uint16_t nodeID)
    {
        frame_.srcNodeID = nodeID;
        return *this;
    }

    MessageBuilder &MessageBuilder::setClass(uint16_t classID)
    {
        frame_.classID = classID;
        return *this;
    }

    MessageBuilder &MessageBuilder::setInstance(uint16_t instanceID)
    {
        frame_.instanceID = instanceID;
        return *this;
    }

    MessageBuilder &MessageBuilder::setAttribute(uint16_t attrID)
    {
        frame_.attrID = attrID;
        return *this;
    }

    MessageBuilder &MessageBuilder::enableCRC(bool enabled)
    {
        frame_.setCRCEnabled(enabled);
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(uint8_t value)
    {
        frame_.payloadType = PayloadType::UINT8;
        frame_.payloadLen = 1;
        frame_.payload.resize(1);
        frame_.payload[0] = value;
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(uint16_t value)
    {
        frame_.payloadType = PayloadType::UINT16;
        frame_.payloadLen = 2;
        frame_.payload.resize(2);
        uint16_t valueBE = utils::hton16(value);
        std::memcpy(frame_.payload.data(), &valueBE, 2);
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(uint32_t value)
    {
        frame_.payloadType = PayloadType::UINT32;
        frame_.payloadLen = 4;
        frame_.payload.resize(4);
        uint32_t valueBE = utils::hton32(value);
        std::memcpy(frame_.payload.data(), &valueBE, 4);
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(uint64_t value)
    {
        frame_.payloadType = PayloadType::UINT64;
        frame_.payloadLen = 8;
        frame_.payload.resize(8);
        uint64_t valueBE = utils::hton64(value);
        std::memcpy(frame_.payload.data(), &valueBE, 8);
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(float value)
    {
        frame_.payloadType = PayloadType::FLOAT32;
        frame_.payloadLen = 4;
        frame_.payload.resize(4);
        uint32_t bits = utils::floatToBits(value);
        uint32_t bitsBE = utils::hton32(bits);
        std::memcpy(frame_.payload.data(), &bitsBE, 4);
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(double value)
    {
        frame_.payloadType = PayloadType::FLOAT64;
        frame_.payloadLen = 8;
        frame_.payload.resize(8);
        uint64_t bits = utils::doubleToBits(value);
        uint64_t bitsBE = utils::hton64(bits);
        std::memcpy(frame_.payload.data(), &bitsBE, 8);
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(const std::string &value)
    {
        frame_.payloadType = PayloadType::STRING;
        frame_.payloadLen = static_cast<uint16_t>(value.size());
        frame_.payload.resize(value.size());
        std::memcpy(frame_.payload.data(), value.data(), value.size());
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(const std::vector<uint8_t> &value)
    {
        frame_.payloadType = PayloadType::OPAQUE;
        frame_.payloadLen = static_cast<uint16_t>(value.size());
        frame_.payload = value;
        return *this;
    }

    MessageBuilder &MessageBuilder::setPayload(const char *value)
    {
        return setPayload(std::string(value));
    }

    MessageBuilder &MessageBuilder::setNoPayload()
    {
        frame_.payloadType = PayloadType::NONE;
        frame_.payloadLen = 0;
        frame_.payload.clear();
        return *this;
    }

    Frame MessageBuilder::build() const
    {
        return frame_;
    }

    MessageBuilder MessageBuilder::request(uint16_t src, uint16_t classID,
                                           uint16_t instanceID, uint16_t attrID)
    {
        MessageBuilder builder;
        builder.setMsgType(MsgType::REQUEST)
            .setSrcNode(src)
            .setClass(classID)
            .setInstance(instanceID)
            .setAttribute(attrID)
            .setNoPayload();
        return builder;
    }

    MessageBuilder MessageBuilder::response(uint16_t src, uint16_t classID,
                                            uint16_t instanceID, uint16_t attrID)
    {
        MessageBuilder builder;
        builder.setMsgType(MsgType::RESPONSE)
            .setSrcNode(src)
            .setClass(classID)
            .setInstance(instanceID)
            .setAttribute(attrID);
        return builder;
    }

    MessageBuilder MessageBuilder::event(uint16_t src, uint16_t classID,
                                         uint16_t instanceID, uint16_t attrID)
    {
        MessageBuilder builder;
        builder.setMsgType(MsgType::EVENT)
            .setSrcNode(src)
            .setClass(classID)
            .setInstance(instanceID)
            .setAttribute(attrID);
        return builder;
    }

    MessageBuilder MessageBuilder::error(uint16_t src, uint16_t classID,
                                         uint16_t instanceID, uint16_t attrID, ErrorCode code)
    {
        MessageBuilder builder;
        builder.setMsgType(MsgType::ERROR)
            .setSrcNode(src)
            .setClass(classID)
            .setInstance(instanceID)
            .setAttribute(attrID)
            .setPayload(static_cast<uint8_t>(code));
        return builder;
    }

    MessageBuilder MessageBuilder::subscribe(uint16_t src, uint16_t classID,
                                             uint16_t instanceID, uint16_t attrID)
    {
        MessageBuilder builder;
        builder.setMsgType(MsgType::SUBSCRIBE)
            .setSrcNode(src)
            .setClass(classID)
            .setInstance(instanceID)
            .setAttribute(attrID)
            .setNoPayload();
        return builder;
    }

    MessageBuilder MessageBuilder::unsubscribe(uint16_t src, uint16_t classID,
                                               uint16_t instanceID, uint16_t attrID)
    {
        MessageBuilder builder;
        builder.setMsgType(MsgType::UNSUBSCRIBE)
            .setSrcNode(src)
            .setClass(classID)
            .setInstance(instanceID)
            .setAttribute(attrID)
            .setNoPayload();
        return builder;
    }

    MessageBuilder MessageBuilder::ack(uint16_t src, uint16_t classID,
                                       uint16_t instanceID, uint16_t attrID)
    {
        MessageBuilder builder;
        builder.setMsgType(MsgType::ACK)
            .setSrcNode(src)
            .setClass(classID)
            .setInstance(instanceID)
            .setAttribute(attrID)
            .setNoPayload();
        return builder;
    }

    // MessageParser Implementation

    MessageParser::MessageParser(const Frame &frame) : frame_(frame)
    {
    }

    std::optional<uint8_t> MessageParser::getUInt8() const
    {
        if (frame_.payloadType != PayloadType::UINT8 || frame_.payload.size() != 1)
        {
            return std::nullopt;
        }
        return frame_.payload[0];
    }

    std::optional<uint16_t> MessageParser::getUInt16() const
    {
        if (frame_.payloadType != PayloadType::UINT16 || frame_.payload.size() != 2)
        {
            return std::nullopt;
        }
        uint16_t valueBE;
        std::memcpy(&valueBE, frame_.payload.data(), 2);
        return utils::ntoh16(valueBE);
    }

    std::optional<uint32_t> MessageParser::getUInt32() const
    {
        if (frame_.payloadType != PayloadType::UINT32 || frame_.payload.size() != 4)
        {
            return std::nullopt;
        }
        uint32_t valueBE;
        std::memcpy(&valueBE, frame_.payload.data(), 4);
        return utils::ntoh32(valueBE);
    }

    std::optional<uint64_t> MessageParser::getUInt64() const
    {
        if (frame_.payloadType != PayloadType::UINT64 || frame_.payload.size() != 8)
        {
            return std::nullopt;
        }
        uint64_t valueBE;
        std::memcpy(&valueBE, frame_.payload.data(), 8);
        return utils::ntoh64(valueBE);
    }

    std::optional<float> MessageParser::getFloat32() const
    {
        if (frame_.payloadType != PayloadType::FLOAT32 || frame_.payload.size() != 4)
        {
            return std::nullopt;
        }
        uint32_t bitsBE;
        std::memcpy(&bitsBE, frame_.payload.data(), 4);
        uint32_t bits = utils::ntoh32(bitsBE);
        return utils::bitsToFloat(bits);
    }

    std::optional<double> MessageParser::getFloat64() const
    {
        if (frame_.payloadType != PayloadType::FLOAT64 || frame_.payload.size() != 8)
        {
            return std::nullopt;
        }
        uint64_t bitsBE;
        std::memcpy(&bitsBE, frame_.payload.data(), 8);
        uint64_t bits = utils::ntoh64(bitsBE);
        return utils::bitsToDouble(bits);
    }

    std::optional<std::string> MessageParser::getString() const
    {
        if (frame_.payloadType != PayloadType::STRING)
        {
            return std::nullopt;
        }
        return std::string(reinterpret_cast<const char *>(frame_.payload.data()),
                           frame_.payload.size());
    }

    std::optional<std::vector<uint8_t>> MessageParser::getOpaque() const
    {
        if (frame_.payloadType != PayloadType::OPAQUE)
        {
            return std::nullopt;
        }
        return frame_.payload;
    }

    PayloadValue MessageParser::getValue() const
    {
        switch (frame_.payloadType)
        {
        case PayloadType::NONE:
            return std::monostate{};
        case PayloadType::UINT8:
            if (auto val = getUInt8())
                return *val;
            break;
        case PayloadType::UINT16:
            if (auto val = getUInt16())
                return *val;
            break;
        case PayloadType::UINT32:
            if (auto val = getUInt32())
                return *val;
            break;
        case PayloadType::UINT64:
            if (auto val = getUInt64())
                return *val;
            break;
        case PayloadType::FLOAT32:
            if (auto val = getFloat32())
                return *val;
            break;
        case PayloadType::FLOAT64:
            if (auto val = getFloat64())
                return *val;
            break;
        case PayloadType::STRING:
            if (auto val = getString())
                return *val;
            break;
        case PayloadType::OPAQUE:
            if (auto val = getOpaque())
                return *val;
            break;
        }
        return std::monostate{};
    }

    std::optional<ErrorCode> MessageParser::getErrorCode() const
    {
        if (frame_.msgType != MsgType::ERROR)
        {
            return std::nullopt;
        }
        auto code = getUInt8();
        if (!code)
        {
            return std::nullopt;
        }
        return static_cast<ErrorCode>(*code);
    }

} // namespace limp
