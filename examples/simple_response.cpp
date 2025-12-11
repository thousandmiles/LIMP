#include <limp/limp.hpp>
#include <iostream>
#include <chrono>

using namespace limp;

int main()
{
    std::cout << "=== LIMP Response Builder Example ===\n\n";

    // Simulate different types of responses

    // 1. Tag Quality Response (UINT8)
    std::cout << "1. Tag Quality Response\n";
    auto qualityResp = MessageBuilder::response(
                           static_cast<uint16_t>(NodeID::PLC),
                           static_cast<uint16_t>(NodeID::HMI),
                           static_cast<uint16_t>(ClassID::Tag),
                           10,
                           TagAttr::Quality)
                           .setPayload(static_cast<uint8_t>(Quality::Good))
                           .enableCRC();

    Frame qFrame = qualityResp.build();
    MessageParser qParser(qFrame);
    if (auto qual = qParser.getUInt8())
    {
        std::cout << "   Quality: " << toString(static_cast<Quality>(*qual)) << "\n\n";
    }

    // 2. Tag Timestamp Response (UINT64)
    std::cout << "2. Tag Timestamp Response\n";
    auto now = std::chrono::system_clock::now();
    auto epochMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch())
                       .count();

    auto timestampResp = MessageBuilder::response(
                             static_cast<uint16_t>(NodeID::PLC),
                             static_cast<uint16_t>(NodeID::HMI),
                             static_cast<uint16_t>(ClassID::Tag),
                             10,
                             TagAttr::Timestamp)
                             .setPayload(static_cast<uint64_t>(epochMs))
                             .enableCRC();

    Frame tFrame = timestampResp.build();
    MessageParser tParser(tFrame);
    if (auto ts = tParser.getUInt64())
    {
        std::cout << "   Timestamp (epoch ms): " << *ts << "\n\n";
    }

    // 3. Motion Position Response (FLOAT64)
    std::cout << "3. Motion Position Response\n";
    auto positionResp = MessageBuilder::response(
                            static_cast<uint16_t>(NodeID::PLC),
                            static_cast<uint16_t>(NodeID::HMI),
                            static_cast<uint16_t>(ClassID::Motion),
                            0, // Motor0
                            MotionAttr::Position)
                            .setPayload(3.14159265359)
                            .enableCRC();

    Frame pFrame = positionResp.build();
    MessageParser pParser(pFrame);
    if (auto pos = pParser.getFloat64())
    {
        std::cout << "   Position: " << *pos << " units\n\n";
    }

    // 4. Alarm Message Response (STRING)
    std::cout << "4. Alarm Message Response\n";
    auto alarmResp = MessageBuilder::response(
                         static_cast<uint16_t>(NodeID::Alarm),
                         static_cast<uint16_t>(NodeID::HMI),
                         static_cast<uint16_t>(ClassID::AlarmObject),
                         5,
                         AlarmAttr::Message)
                         .setPayload("Temperature exceeded threshold")
                         .enableCRC();

    Frame aFrame = alarmResp.build();
    MessageParser aParser(aFrame);
    if (auto msg = aParser.getString())
    {
        std::cout << "   Message: \"" << *msg << "\"\n\n";
    }

    // 5. Binary Data Response (OPAQUE)
    std::cout << "5. Binary Data Response\n";
    std::vector<uint8_t> binaryData = {0xDE, 0xAD, 0xBE, 0xEF, 0xCA, 0xFE};

    auto binaryResp = MessageBuilder::response(
                          static_cast<uint16_t>(NodeID::Server),
                          static_cast<uint16_t>(NodeID::HMI),
                          static_cast<uint16_t>(ClassID::System),
                          0,
                          0x0010 // Custom attribute
                          )
                          .setPayload(binaryData)
                          .enableCRC();

    Frame bFrame = binaryResp.build();
    MessageParser bParser(bFrame);
    if (auto data = bParser.getOpaque())
    {
        std::cout << "   Binary data (" << data->size() << " bytes): ";
        for (auto byte : *data)
        {
            printf("%02X ", byte);
        }
        std::cout << "\n\n";
    }

    // 6. EVENT message (value change notification)
    std::cout << "6. EVENT - Tag Value Changed\n";
    auto eventMsg = MessageBuilder::event(
                        static_cast<uint16_t>(NodeID::PLC),
                        static_cast<uint16_t>(NodeID::HMI),
                        static_cast<uint16_t>(ClassID::Tag),
                        7,
                        TagAttr::Value)
                        .setPayload(456.78f)
                        .enableCRC();

    Frame eFrame = eventMsg.build();
    MessageParser eParser(eFrame);
    std::cout << "   MsgType: " << toString(eParser.msgType()) << "\n";
    if (auto val = eParser.getFloat32())
    {
        std::cout << "   New Value: " << *val << "\n\n";
    }

    std::cout << "=== All Response Examples Complete ===\n";

    return 0;
}
