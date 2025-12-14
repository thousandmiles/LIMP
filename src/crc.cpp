#include "limp/crc.hpp"

namespace limp
{

    uint16_t calculateCRC16(const uint8_t *data, size_t length)
    {
        uint16_t crc = CRC16_INITIAL; // Start with 0xFFFF for Modbus

        for (size_t i = 0; i < length; ++i)
        {
            crc ^= static_cast<uint16_t>(data[i]); // XOR with LSB (Modbus style)

            for (int j = 0; j < 8; ++j)
            {
                if (crc & 0x0001) // Check LSB (reflected)
                {
                    crc = (crc >> 1) ^ CRC16_POLYNOMIAL; // Shift right and XOR , 0xA001;
                }
                else
                {
                    crc >>= 1; // Just shift right
                }
            }
        }

        return crc;
    }

    bool verifyCRC16(const uint8_t *data, size_t length)
    {
        if (length < 2)
        {
            return false;
        }

        // Calculate CRC over all bytes except the last 2 (which contain the CRC)
        uint16_t calculated = calculateCRC16(data, length - 2);

        // Extract the stored CRC (big-endian)
        uint16_t stored = (static_cast<uint16_t>(data[length - 2]) << 8) |
                          static_cast<uint16_t>(data[length - 1]);

        return calculated == stored;
    }

} // namespace limp
