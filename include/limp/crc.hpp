#pragma once

#include <cstdint>
#include <cstddef>

namespace limp
{

    constexpr uint16_t CRC16_POLYNOMIAL = 0xA001;
    constexpr uint16_t CRC16_INITIAL = 0xFFFF;

    /**
     * Calculate CRC16-MODBUS checksum
     * Polynomial: 0xA001 (reflected), Initial: 0xFFFF, LSB-first
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
