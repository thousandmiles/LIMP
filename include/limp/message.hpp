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
     * @brief Fluent API for constructing LIMP messages
     *
     * Type-safe builder pattern for creating LIMP protocol frames.
     * All setter methods return *this to enable method chaining.
     * Use factory methods (request, response, event, etc.) for common message types.
     *
     * @example
     * @code
     * // Request tag value from PLC
     * auto request = MessageBuilder::request(0x10, 0x3000, 7, 0x01)
     *     .enableCRC()
     *     .build();
     *
     * // Respond with float value
     * auto response = MessageBuilder::response(0x30, 0x3000, 7, 0x01)
     *     .setPayload(123.45f)
     *     .enableCRC()
     *     .build();
     * @endcode
     */
    class MessageBuilder
    {
    public:
        /** @brief Default constructor */
        MessageBuilder();

        // Copy operations (default behavior is acceptable)
        MessageBuilder(const MessageBuilder &) = default;
        MessageBuilder &operator=(const MessageBuilder &) = default;

        // Move operations (efficient transfer of internal frame)
        MessageBuilder(MessageBuilder &&) noexcept = default;
        MessageBuilder &operator=(MessageBuilder &&) noexcept = default;

        /** @brief Destructor */
        ~MessageBuilder() = default;

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

        /** @brief Set STRING payload (move version) */
        MessageBuilder &setPayload(std::string &&value);

        /** @brief Set OPAQUE (binary) payload */
        MessageBuilder &setPayload(const std::vector<uint8_t> &value);

        /** @brief Set OPAQUE (binary) payload (move version) */
        MessageBuilder &setPayload(std::vector<uint8_t> &&value);

        /** @brief Set STRING payload from C-string */
        MessageBuilder &setPayload(const char *value);

        /** @brief Clear payload (set to NONE) */
        MessageBuilder &setNoPayload();

        /** @} */

        /**
         * @brief Build final Frame object (copy)
         * @return Complete LIMP frame ready for serialization
         */
        Frame build() const &;

        /**
         * @brief Build final Frame object (move)
         * @return Complete LIMP frame ready for serialization (moved)
         */
        Frame build() &&;

        /**
         * @name Factory Methods
         * Convenience methods for creating common message types
         * @{
         */

        /**
         * @brief Create REQUEST message builder
         *
         * Pre-filled: msgType=REQUEST, srcNode, classID, instanceID, attrID
         * User should: call setPayload() if data needed, optionally enableCRC(), then build()
         *
         * @param src Source node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as REQUEST
         */
        static MessageBuilder request(uint16_t src, uint16_t classID,
                                      uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create RESPONSE message builder
         *
         * Pre-filled: msgType=RESPONSE, srcNode, classID, instanceID, attrID
         * User should: call setPayload() with response data, optionally enableCRC(), then build()
         *
         * @param src Source node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as RESPONSE
         */
        static MessageBuilder response(uint16_t src, uint16_t classID,
                                       uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create EVENT message builder
         *
         * Pre-filled: msgType=EVENT, srcNode, classID, instanceID, attrID
         * User should: call setPayload() with event data, optionally enableCRC(), then build()
         *
         * @param src Source node ID
         * @param classID Object class ID
         * @param instanceID Object instance ID
         * @param attrID Attribute ID
         * @return MessageBuilder configured as EVENT
         */
        static MessageBuilder event(uint16_t src, uint16_t classID,
                                    uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create ERROR message builder
         *
         * Pre-filled: msgType=ERROR, srcNode, classID, instanceID, attrID
         * User should: call setPayload() with application error code (uint8_t), then build()
         *
         * Note: Applications define their own error enums and encode as uint8_t payload.
         *
         * @param src Source node ID
         * @param classID Object class ID (echo from original request)
         * @param instanceID Object instance ID (echo from original request)
         * @param attrID Attribute ID (echo from original request)
         * @return MessageBuilder configured as ERROR
         */
        static MessageBuilder error(uint16_t src, uint16_t classID,
                                    uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create SUBSCRIBE message builder
         *
         * Pre-filled: msgType=SUBSCRIBE, srcNode, classID, instanceID, attrID
         * User should: typically call build() directly (no payload needed for subscription)
         *
         * @param src Source node ID
         * @param classID Object class ID to subscribe to
         * @param instanceID Object instance ID to subscribe to
         * @param attrID Attribute ID to subscribe to
         * @return MessageBuilder configured as SUBSCRIBE
         */
        static MessageBuilder subscribe(uint16_t src, uint16_t classID,
                                        uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create UNSUBSCRIBE message builder
         *
         * Pre-filled: msgType=UNSUBSCRIBE, srcNode, classID, instanceID, attrID
         * User should: typically call build() directly (no payload needed for unsubscription)
         *
         * @param src Source node ID
         * @param classID Object class ID to unsubscribe from
         * @param instanceID Object instance ID to unsubscribe from
         * @param attrID Attribute ID to unsubscribe from
         * @return MessageBuilder configured as UNSUBSCRIBE
         */
        static MessageBuilder unsubscribe(uint16_t src, uint16_t classID,
                                          uint16_t instanceID, uint16_t attrID);

        /**
         * @brief Create ACK message builder
         *
         * Pre-filled: msgType=ACK, srcNode, classID, instanceID, attrID
         * User should: typically call build() directly (no payload needed for acknowledgment)
         *
         * @param src Source node ID
         * @param classID Object class ID (echo from original message)
         * @param instanceID Object instance ID (echo from original message)
         * @param attrID Attribute ID (echo from original message)
         * @return MessageBuilder configured as ACK
         */
        static MessageBuilder ack(uint16_t src, uint16_t classID,
                                  uint16_t instanceID, uint16_t attrID);

        /** @} */

    private:
        Frame frame_;
    };

    /**
     * @brief Type-safe parser for extracting frame payload
     *
     * Provides convenient typed access to frame payload data.
     * All getters return std::optional (empty if type mismatch).
     * Helper methods provide message type checking.
     *
     * @example
     * @code
     * MessageParser parser(receivedFrame);
     * if (parser.isResponse()) {
     *     if (auto val = parser.getFloat32()) {
     *         std::cout << "Temperature: " << *val << "°C\n";
     *     }
     * }
     * @endcode
     */
    class MessageParser
    {
    public:
        /**
         * @brief Construct parser from frame (copy)
         * @param frame LIMP frame to parse
         */
        explicit MessageParser(const Frame &frame);

        /**
         * @brief Construct parser from frame (move)
         * @param frame LIMP frame to parse (moved)
         */
        explicit MessageParser(Frame &&frame) noexcept;

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
        const Frame &frame() const noexcept { return frame_; }

        /** @brief Get message type */
        MsgType msgType() const noexcept { return frame_.msgType; }

        /** @brief Get source node ID */
        uint16_t srcNode() const noexcept { return frame_.srcNodeID; }

        /** @brief Get class ID */
        uint16_t classID() const noexcept { return frame_.classID; }

        /** @brief Get instance ID */
        uint16_t instanceID() const noexcept { return frame_.instanceID; }

        /** @brief Get attribute ID */
        uint16_t attrID() const noexcept { return frame_.attrID; }

        /** @brief Get payload type */
        PayloadType payloadType() const noexcept { return frame_.payloadType; }

        /** @} */

        /**
         * @name Helper Methods
         * Convenient message type checks
         * @{
         */

        /** @brief Check if message is REQUEST */
        bool isRequest() const noexcept { return frame_.msgType == MsgType::REQUEST; }

        /** @brief Check if message is RESPONSE */
        bool isResponse() const noexcept { return frame_.msgType == MsgType::RESPONSE; }

        /** @brief Check if message is EVENT */
        bool isEvent() const noexcept { return frame_.msgType == MsgType::EVENT; }

        /** @brief Check if message is ERROR */
        bool isError() const noexcept { return frame_.msgType == MsgType::ERROR; }

        /**
         * @brief Extract application error code from ERROR message
         *
         * Convenience method for ERROR messages. Extracts first payload byte as error code.
         * Applications should define their own error enums and encode as uint8_t.
         *
         * @return uint8_t error code if message is ERROR type with payload, empty otherwise
         */
        std::optional<uint8_t> getErrorCode() const;

        /** @} */

    private:
        Frame frame_;
    };

} // namespace limp
