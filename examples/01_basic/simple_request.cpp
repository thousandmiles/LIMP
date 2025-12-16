#include <limp/limp.hpp>
#include <iostream>
#include <iomanip>

using namespace limp;

void printFrame(const Frame &frame)
{
    std::cout << "Frame Details:\n";
    std::cout << "  Version: 0x" << std::hex << static_cast<int>(frame.version) << std::dec << "\n";
    std::cout << "  MsgType: " << toString(frame.msgType) << "\n";
    std::cout << "  SrcNode: 0x" << std::hex << frame.srcNodeID << std::dec << "\n";
    std::cout << "  ClassID: 0x" << std::hex << frame.classID << std::dec << "\n";
    std::cout << "  InstanceID: 0x" << std::hex << frame.instanceID << std::dec << "\n";
    std::cout << "  AttrID: 0x" << std::hex << frame.attrID << std::dec << "\n";
    std::cout << "  PayloadType: " << toString(frame.payloadType) << "\n";
    std::cout << "  PayloadLen: " << frame.payloadLen << "\n";
    std::cout << "  CRC Enabled: " << (frame.hasCRC() ? "Yes" : "No") << "\n";
    std::cout << "  Total Size: " << frame.totalSize() << " bytes\n";
}

void printBuffer(const std::vector<uint8_t> &buffer)
{
    std::cout << "Binary Frame (hex):\n  ";
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        std::cout << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(buffer[i]) << " ";
        if ((i + 1) % 16 == 0)
            std::cout << "\n  ";
    }
    std::cout << std::dec << "\n\n";
}

int main()
{
    std::cout << "=== LIMP Simple Request Example ===\n\n";
    std::cout << "Library Version: " << limp::VERSION << "\n\n";

    // Example 1: Simple REQUEST without payload
    std::cout << "Example 1: HMI requests PLC Tag[7].Value\n";
    std::cout << "----------------------------------------\n";

    auto request = MessageBuilder::request(
        0x0010, // Source: HMI
        0x3000, // Class: Tag
        7,      // Instance: Tag7
        0x0001  // Attribute: Value
    );

    Frame frame1 = request.build();
    printFrame(frame1);

    // Serialize to binary
    std::vector<uint8_t> buffer1;
    if (serializeFrame(frame1, buffer1))
    {
        std::cout << "\nSerialization successful!\n";
        printBuffer(buffer1);
    }
    else
    {
        std::cout << "Serialization failed!\n";
        return 1;
    }

    // Deserialize back
    Frame frame1b;
    if (deserializeFrame(buffer1, frame1b))
    {
        std::cout << "Deserialization successful!\n";
        std::cout << "Frame validation: " << (frame1b.validate() ? "PASS" : "FAIL") << "\n\n";
    }
    else
    {
        std::cout << "Deserialization failed!\n";
        return 1;
    }

    // Example 2: RESPONSE with float32 payload and CRC
    std::cout << "\nExample 2: PLC responds with float32=123.45 (with CRC)\n";
    std::cout << "------------------------------------------------------\n";

    auto response = MessageBuilder::response(
                        0x0030, // Source: PLC
                        0x3000, // Class: Tag
                        7,      // Instance: Tag7
                        0x0001  // Attribute: Value
                        )
                        .setPayload(123.45f)
                        .enableCRC(true);

    Frame frame2 = response.build();
    printFrame(frame2);

    std::vector<uint8_t> buffer2;
    if (serializeFrame(frame2, buffer2))
    {
        std::cout << "\nSerialization successful!\n";
        printBuffer(buffer2);
    }

    // Parse response
    MessageParser parser(frame2);
    if (auto value = parser.getFloat32())
    {
        std::cout << "Parsed float32 value: " << *value << "\n\n";
    }

    // Example 3: ERROR response
    std::cout << "\nExample 3: ERROR - Invalid Attribute\n";
    std::cout << "------------------------------------\n";

    auto error = MessageBuilder::error(
        0x0030,                     // Source: PLC
        0x3000,                     // Class: Tag (echoed)
        7,                          // Instance: Tag7 (echoed)
        0x0001,                     // Attribute: Value (echoed)
        ErrorCode::InvalidAttribute // Error code
    );

    Frame frame3 = error.build();
    printFrame(frame3);

    MessageParser errorParser(frame3);
    if (auto code = errorParser.getErrorCode())
    {
        std::cout << "Error Code: " << toString(*code) << "\n\n";
    }

    // Example 4: SUBSCRIBE request
    std::cout << "\nExample 4: SUBSCRIBE to Tag updates\n";
    std::cout << "-----------------------------------\n";

    auto subscribe = MessageBuilder::subscribe(
        0x0010,
        0x3000,
        7,
        0x0001);

    Frame frame4 = subscribe.build();
    printFrame(frame4);

    std::cout << "\n=== Examples Complete ===\n";

    return 0;
}
