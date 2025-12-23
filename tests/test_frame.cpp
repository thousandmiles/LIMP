#include <limp/limp.hpp>
#include <iostream>
#include <cassert>

using namespace limp;

void testBasicFrame()
{
    std::cout << "Test: Basic Frame Creation and Serialization... ";

    auto frame = MessageBuilder::request(
                     0x0010,
                     0x3000,
                     7,
                     0x0001)
                     .build();

    assert(frame.version == PROTOCOL_VERSION);
    assert(frame.msgType == MsgType::REQUEST);
    assert(frame.srcNodeID == 0x0010);

    assert(frame.validate());

    std::vector<uint8_t> buffer;
    assert(serializeFrame(frame, buffer));
    assert(buffer.size() == MIN_FRAME_SIZE);

    Frame frame2;
    assert(deserializeFrame(buffer, frame2));
    assert(frame2.validate());
    assert(frame2.msgType == frame.msgType);
    assert(frame2.srcNodeID == frame.srcNodeID);

    std::cout << "PASS\n";
}

void testPayloadTypes()
{
    std::cout << "Test: Payload Types... ";

    // UINT8
    auto f1 = MessageBuilder::response(0x10, 0x3000, 1, 1)
                  .setPayload(static_cast<uint8_t>(42))
                  .build();
    assert(f1.payloadType == PayloadType::UINT8);
    MessageParser p1(f1);
    assert(p1.getUInt8().value() == 42);

    // UINT32
    auto f2 = MessageBuilder::response(0x10, 0x3000, 1, 1)
                  .setPayload(static_cast<uint32_t>(12345678))
                  .build();
    MessageParser p2(f2);
    assert(p2.getUInt32().value() == 12345678);

    // FLOAT32
    auto f3 = MessageBuilder::response(0x10, 0x3000, 1, 1)
                  .setPayload(123.45f)
                  .build();
    MessageParser p3(f3);
    float val = p3.getFloat32().value();
    assert(val > 123.44f && val < 123.46f);

    // STRING
    auto f4 = MessageBuilder::response(0x10, 0x3000, 1, 1)
                  .setPayload("Hello LIMP")
                  .build();
    MessageParser p4(f4);
    assert(p4.getString().value() == "Hello LIMP");

    std::cout << "PASS\n";
}

void testCRC()
{
    std::cout << "Test: CRC Calculation and Verification... ";

    auto frame = MessageBuilder::response(0x10, 0x3000, 1, 1)
                     .setPayload(123.45f)
                     .enableCRC(true)
                     .build();

    assert(frame.hasCRC());

    std::vector<uint8_t> buffer;
    assert(serializeFrame(frame, buffer));
    assert(buffer.size() == MIN_FRAME_SIZE + 4 + CRC_SIZE); // header + float32 + crc

    // Verify CRC
    assert(verifyCRC16(buffer.data(), buffer.size()));

    // Corrupt a byte
    buffer[10] ^= 0xFF;
    assert(!verifyCRC16(buffer.data(), buffer.size()));

    std::cout << "PASS\n";
}

void testErrorMessages()
{
    std::cout << "Test: Error Messages... ";

    // Application defines its own error codes
    enum class AppError : uint8_t {
        InvalidAttribute = 0x03
    };

    auto error = MessageBuilder::error(
                     0x0030,
                     0x3000,
                     7,
                     0x0001)
                     .setPayload(static_cast<uint8_t>(AppError::InvalidAttribute))
                     .build();

    assert(error.msgType == MsgType::ERROR);

    MessageParser parser(error);
    assert(parser.isError());
    auto code = parser.getUInt8();
    assert(code.has_value());
    assert(code.value() == static_cast<uint8_t>(AppError::InvalidAttribute));

    std::cout << "PASS\n";
}

void testEndianness()
{
    std::cout << "Test: Endianness Conversion... ";

    auto frame = MessageBuilder::response(0x1234, 0xABCD, 0xEF01, 0x2345)
                     .setPayload(static_cast<uint32_t>(0xDEADBEEF))
                     .build();

    std::vector<uint8_t> buffer;
    assert(serializeFrame(frame, buffer));

    // Check big-endian encoding in buffer
    // SrcNodeID at offset 2-3
    assert(buffer[2] == 0x12);
    assert(buffer[3] == 0x34);

    // Payload (UINT32) at new offset due to header shrink (14 bytes)
    assert(buffer[14] == 0xDE);
    assert(buffer[15] == 0xAD);
    assert(buffer[16] == 0xBE);
    assert(buffer[17] == 0xEF);

    // Deserialize and verify
    Frame frame2;
    assert(deserializeFrame(buffer, frame2));
    assert(frame2.srcNodeID == 0x1234);

    MessageParser parser(frame2);
    assert(parser.getUInt32().value() == 0xDEADBEEF);

    std::cout << "PASS\n";
}

void testMessageTypes()
{
    std::cout << "Test: All Message Types... ";

    auto req = MessageBuilder::request(0x10, 0x3000, 1, 1).build();
    assert(req.msgType == MsgType::REQUEST);

    auto resp = MessageBuilder::response(0x20, 0x3000, 1, 1).build();
    assert(resp.msgType == MsgType::RESPONSE);

    auto evt = MessageBuilder::event(0x30, 0x3000, 1, 1).build();
    assert(evt.msgType == MsgType::EVENT);

    auto sub = MessageBuilder::subscribe(0x10, 0x3000, 1, 1).build();
    assert(sub.msgType == MsgType::SUBSCRIBE);

    auto unsub = MessageBuilder::unsubscribe(0x10, 0x3000, 1, 1).build();
    assert(unsub.msgType == MsgType::UNSUBSCRIBE);

    auto ack = MessageBuilder::ack(0x20, 0x3000, 1, 1).build();
    assert(ack.msgType == MsgType::ACK);

    std::cout << "PASS\n";
}

int main()
{
    std::cout << "=== LIMP Frame Tests ===\n\n";

    try
    {
        testBasicFrame();
        testPayloadTypes();
        testCRC();
        testErrorMessages();
        testEndianness();
        testMessageTypes();

        std::cout << "\n=== All Tests Passed ===\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cout << "FAIL: " << e.what() << "\n";
        return 1;
    }
}
