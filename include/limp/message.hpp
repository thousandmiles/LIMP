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
     * @brief Message Builder - Fluent API for constructing LIMP messages
     *
     * Provides a type-safe builder pattern for creating LIMP protocol frames.
     * All setters return *this for method chaining.
     *
     * @example
     * @code
     * auto msg = MessageBuilder::request(0x10, 0x30, 0x3000, 1, 1)
     *     .setPayload(42.5f)
     *     .enableCRC(true)
     *     .build();
     * @endcode
     */
    class MessageBuilder
    {
    public:
        /** @brief Default constructor */
        MessageBuilder();

        /**
         * @brief Set protocol version (default: 0x01)
         * @param version Protocol version
         * @return Reference to builder for chaining
         */
        MessageBuilder &setVersion(uint8_t version);

        /**
         * @brief Set message type
         * @param type Message type (REQUEST, RESPONSE, EVENT, etc.)
         * @return Reference to builder for chaining
         */
        MessageBuilder &setMsgType(MsgType type);

        /**
         * @brief Set source node ID
         * @param nodeID Source node identifier
         * @return Reference to builder for chaining
         */
        MessageBuilder &setSrcNode(uint16_t nodeID);

        /**
         * @brief Set destination node ID
         * @param nodeID Destination node identifier
         * @return Reference to builder for chaining
         */
        MessageBuilder &setDstNode(uint16_t nodeID);

        /**
         * @brief Set class ID
         * @param classID Object class identifier
         * @return Reference to builder for chaining
         */
        MessageBuilder &setClass(uint16_t classID);

        /**
         * @brief Set instance ID
         * @param instanceID Object instance identifier
         * @return Reference to builder for chaining
         */
        MessageBuilder &setInstance(uint16_t instanceID);

        /**
         * @brief Set attribute ID
         * @param attrID Attribute identifier
         * @return Reference to builder for chaining
         */
        MessageBuilder &setAttribute(uint16_t attrID);

        /**
         * @brief Enable/disable CRC16-MODBUS validation
         * @param enabled true to enable CRC (default), false to disable
         * @return Reference to builder for chaining
         */
        MessageBuilder &enableCRC(bool enabled = true);

        /**
         * @name Payload Setters
         * Set typed payload data. Type is automatically detected.
         * @{
         */

        /** @brief Set UINT8 payload */
        MessageBuilder &setPayload(uint8_t value);

        /** @brief Set UINT16 payload */
        MessageBuilder &setPayload(uint16_t value);

        /** @brief Set UINT32 payload */
        MessageBuilder &setPayload(uint32_t value);

        /** @brief Set UINT64 payload */
        MessageBuilder &setPayload(uint64_t value);

        /** @brief Set FLOAT32 payload */
        MessageBuilder &setPayload(float value);

        /** @brief Set FLOAT64 payload */
        MessageBuilder &setPayload(double value);

        /** @brief Set STRING payload */
        MessageBuilder &setPayload(const std::string &value);

        /** @brief Set OPAQUE (binary) payload */
        MessageBuilder &setPayload(const std::vector<uint8_t> &value);

        /** @brief Set STRING payload from C-string */
        MessageBuilder &setPayload(const char *value);

        /** @brief Clear payload (set to NONE) */
        MessageBuilder &setNoPayload();

        /** @} */

        /**
         * @brief Build final Frame object
         * @return Complete LIMP frame ready for serialization
         */
        Frame build() const;

        /**
         * @name Factory Methods
         * Convenience methods for creating common message types
         * @{
         */

        /**
         * @brief Create REQUEST message builder
         * @param src Source node ID
         * @param dst Destination node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as REQUEST
         */
        static MessageBuilder request(uint16_t src, uint16_t dst, uint16_t classID,
                                      uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create RESPONSE message builder
         * @param src Source node ID
         * @param dst Destination node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as RESPONSE
         */
        static MessageBuilder response(uint16_t src, uint16_t dst, uint16_t classID,
                                       uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create EVENT message builder
         * @param src Source node ID
         * @param dst Destination node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as EVENT
         */
        static MessageBuilder event(uint16_t src, uint16_t dst, uint16_t classID,
                                    uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create ERROR message builder
         * @param src Source node ID
         * @param dst Destination node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @param code Error code
         * @return MessageBuilder configured as ERROR
         */
        static MessageBuilder error(uint16_t src, uint16_t dst, uint16_t classID,
                                    uint16_t instanceID, uint16_t attrID, ErrorCode code);

        /**
         * @brief Create SUBSCRIBE message builder
         * @param src Source node ID
         * @param dst Destination node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as SUBSCRIBE
         */
        static MessageBuilder subscribe(uint16_t src, uint16_t dst, uint16_t classID,
                                        uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create UNSUBSCRIBE message builder
         * @param src Source node ID
         * @param dst Destination node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as UNSUBSCRIBE
         */
        static MessageBuilder unsubscribe(uint16_t src, uint16_t dst, uint16_t classID,
                                          uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create ACK message builder
         * @param src Source node ID
         * @param dst Destination node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as ACK
         */
        static MessageBuilder ack(uint16_t src, uint16_t dst, uint16_t classID,
                                  uint16_t instanceID, uint16_t attrID);

        /** @} */

    private:
        Frame frame_;
    };

    /**
     * @brief Message Parser - Extract typed payload from frames
     *
     * Provides type-safe access to frame payload data.
     * Returns std::optional - empty if type mismatch or no payload.
     *
     * @example
     * @code
     * MessageParser parser(frame);
     * if (auto val = parser.getFloat32()) {
     *     std::cout << "Value: " << *val << std::endl;
     * }
     * @endcode
     */
    class MessageParser
    {
    public:
        /**
         * @brief Construct parser from frame
         * @param frame LIMP frame to parse
         */
        explicit MessageParser(const Frame &frame);

        /**
         * @name Typed Payload Getters
         * Extract payload as specific type. Returns empty optional if type mismatch.
         * @{
         */

        /** @brief Get UINT8 payload */
        std::optional<uint8_t> getUInt8() const;

        /** @brief Get UINT16 payload */
        std::optional<uint16_t> getUInt16() const;

        /** @brief Get UINT32 payload */
        std::optional<uint32_t> getUInt32() const;

        /** @brief Get UINT64 payload */
        std::optional<uint64_t> getUInt64() const;

        /** @brief Get FLOAT32 payload */
        std::optional<float> getFloat32() const;

        /** @brief Get FLOAT64 payload */
        std::optional<double> getFloat64() const;

        /** @brief Get STRING payload */
        std::optional<std::string> getString() const;

        /** @brief Get OPAQUE (binary) payload */
        std::optional<std::vector<uint8_t>> getOpaque() const;

        /** @} */

        /**
         * @brief Get payload as variant type
         * @return PayloadValue containing the actual payload
         */
        PayloadValue getValue() const;

        /**
         * @name Frame Accessors
         * Direct access to frame header fields
         * @{
         */

        /** @brief Get underlying frame */
        const Frame &frame() const { return frame_; }

        /** @brief Get message type */
        MsgType msgType() const { return frame_.msgType; }

        /** @brief Get source node ID */
        uint16_t srcNode() const { return frame_.srcNodeID; }

        /** @brief Get destination node ID */
        uint16_t dstNode() const { return frame_.dstNodeID; }

        /** @brief Get class ID */
        uint16_t classID() const { return frame_.classID; }

        /** @brief Get instance ID */
        uint16_t instanceID() const { return frame_.instanceID; }

        /** @brief Get attribute ID */
        uint16_t attrID() const { return frame_.attrID; }

        /** @brief Get payload type */
        PayloadType payloadType() const { return frame_.payloadType; }

        /** @} */

        /**
         * @name Helper Methods
         * Convenient message type checks
         * @{
         */

        /** @brief Check if message is REQUEST */
        bool isRequest() const { return frame_.msgType == MsgType::REQUEST; }

        /** @brief Check if message is RESPONSE */
        bool isResponse() const { return frame_.msgType == MsgType::RESPONSE; }

        /** @brief Check if message is EVENT */
        bool isEvent() const { return frame_.msgType == MsgType::EVENT; }

        /** @brief Check if message is ERROR */
        bool isError() const { return frame_.msgType == MsgType::ERROR; }

        /**
         * @brief Extract error code from ERROR message
         * @return ErrorCode if message is ERROR type, empty otherwise
         */
        std::optional<ErrorCode> getErrorCode() const;

        /** @} */

    private:
        Frame frame_;
    };

} // namespace limp
