#pragma once

#include <cstdint>
#include <cstddef>

namespace limp
{

    constexpr uint16_t CRC16_POLYNOMIAL = 0xA001;
    constexpr uint16_t CRC16_INITIAL = 0xFFFF;

    /**
     * @brief Calculate CRC16-MODBUS checksum
     *
     * Algorithm: Polynomial 0xA001 (reflected), Initial value 0xFFFF, LSB-first.
     * Standard MODBUS CRC-16 implementation.
     *
     * @param data Pointer to data buffer
     * @param length Data length in bytes
     * @return 16-bit CRC checksum value
     */
    uint16_t calculateCRC16(const uint8_t *data, size_t length);

    /**
     * @brief Verify CRC16-MODBUS checksum
     *
     * Validates data integrity by checking appended CRC.
     *
     * @param data Pointer to data buffer (including 2-byte CRC at end)
     * @param length Total buffer length including CRC bytes
     * @return true if CRC valid, false if mismatch
     */
    bool verifyCRC16(const uint8_t *data, size_t length);

} // namespace limp
