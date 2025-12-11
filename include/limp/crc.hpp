#pragma once

#include <cstdint>
#include <cstddef>

namespace limp
{

    // CRC16-IBM polynomial 0x8005
    constexpr uint16_t CRC16_POLYNOMIAL = 0x8005;

    /**
     * Calculate CRC16-IBM checksum
     * Polynomial: 0x8005, Initial: 0x0000, no reflection
     *
     * @param data Pointer to data buffer
     * @param length Length of data in bytes
     * @return CRC16 checksum
     */
    uint16_t calculateCRC16(const uint8_t *data, size_t length);

    /**
     * Verify CRC16 checksum
     *
     * @param data Pointer to data buffer (including CRC at end)
     * @param length Total length including CRC
     * @return true if CRC is valid
     */
    bool verifyCRC16(const uint8_t *data, size_t length);

} // namespace limp
