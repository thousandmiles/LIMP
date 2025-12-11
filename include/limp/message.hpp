#pragma once

#include "frame.hpp"
#include "types.hpp"
#include <string>
#include <variant>
#include <memory>

namespace limp
{

    /**
     * Payload value variant type
     * Represents different payload types in a type-safe way
     */
    using PayloadValue = std::variant<
        std::monostate,      // NONE
        uint8_t,             // UINT8
        uint16_t,            // UINT16
        uint32_t,            // UINT32
        uint64_t,            // UINT64
        float,               // FLOAT32
        double,              // FLOAT64
        std::string,         // STRING
        std::vector<uint8_t> // OPAQUE
        >;

    /**
     * Message Builder - Fluent API for constructing LIMP messages
     */
    class MessageBuilder
    {
    public:
        MessageBuilder();

        // Header setters (fluent interface)
        MessageBuilder &setVersion(uint8_t version);
        MessageBuilder &setMsgType(MsgType type);
        MessageBuilder &setSrcNode(uint16_t nodeID);
        MessageBuilder &setDstNode(uint16_t nodeID);
        MessageBuilder &setClass(uint16_t classID);
        MessageBuilder &setInstance(uint16_t instanceID);
        MessageBuilder &setAttribute(uint16_t attrID);
        MessageBuilder &enableCRC(bool enabled = true);

        // Payload setters
        MessageBuilder &setPayload(uint8_t value);
        MessageBuilder &setPayload(uint16_t value);
        MessageBuilder &setPayload(uint32_t value);
        MessageBuilder &setPayload(uint64_t value);
        MessageBuilder &setPayload(float value);
        MessageBuilder &setPayload(double value);
        MessageBuilder &setPayload(const std::string &value);
        MessageBuilder &setPayload(const std::vector<uint8_t> &value);
        MessageBuilder &setPayload(const char *value);
        MessageBuilder &setNoPayload();

        // Build the frame
        Frame build() const;

        // Convenience static methods for common message types
        static MessageBuilder request(uint16_t src, uint16_t dst, uint16_t classID,
                                      uint16_t instanceID, uint16_t attrID);
        static MessageBuilder response(uint16_t src, uint16_t dst, uint16_t classID,
                                       uint16_t instanceID, uint16_t attrID);
        static MessageBuilder event(uint16_t src, uint16_t dst, uint16_t classID,
                                    uint16_t instanceID, uint16_t attrID);
        static MessageBuilder error(uint16_t src, uint16_t dst, uint16_t classID,
                                    uint16_t instanceID, uint16_t attrID, ErrorCode code);
        static MessageBuilder subscribe(uint16_t src, uint16_t dst, uint16_t classID,
                                        uint16_t instanceID, uint16_t attrID);
        static MessageBuilder unsubscribe(uint16_t src, uint16_t dst, uint16_t classID,
                                          uint16_t instanceID, uint16_t attrID);
        static MessageBuilder ack(uint16_t src, uint16_t dst, uint16_t classID,
                                  uint16_t instanceID, uint16_t attrID);

    private:
        Frame frame_;
    };

    /**
     * Message Parser - Extract typed payload from frames
     */
    class MessageParser
    {
    public:
        explicit MessageParser(const Frame &frame);

        // Get typed payload
        std::optional<uint8_t> getUInt8() const;
        std::optional<uint16_t> getUInt16() const;
        std::optional<uint32_t> getUInt32() const;
        std::optional<uint64_t> getUInt64() const;
        std::optional<float> getFloat32() const;
        std::optional<double> getFloat64() const;
        std::optional<std::string> getString() const;
        std::optional<std::vector<uint8_t>> getOpaque() const;

        // Get generic payload value
        PayloadValue getValue() const;

        // Frame accessors
        const Frame &frame() const { return frame_; }
        MsgType msgType() const { return frame_.msgType; }
        uint16_t srcNode() const { return frame_.srcNodeID; }
        uint16_t dstNode() const { return frame_.dstNodeID; }
        uint16_t classID() const { return frame_.classID; }
        uint16_t instanceID() const { return frame_.instanceID; }
        uint16_t attrID() const { return frame_.attrID; }
        PayloadType payloadType() const { return frame_.payloadType; }

        // Helper methods
        bool isRequest() const { return frame_.msgType == MsgType::REQUEST; }
        bool isResponse() const { return frame_.msgType == MsgType::RESPONSE; }
        bool isEvent() const { return frame_.msgType == MsgType::EVENT; }
        bool isError() const { return frame_.msgType == MsgType::ERROR; }
        std::optional<ErrorCode> getErrorCode() const;

    private:
        Frame frame_;
    };

} // namespace limp
