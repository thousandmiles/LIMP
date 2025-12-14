#pragma once

#include <cstdint>
#include <cstring>

namespace limp
{
    /**
     * @brief Utility functions for binary data conversion
     *
     * Provides endianness conversion and floating-point bit manipulation
     * functions for network protocol serialization.
     */
    namespace utils
    {

        /**
         * @brief Convert 16-bit host to network byte order (big-endian)
         * @param value Value in host byte order
         * @return Value in network byte order (big-endian)
         */
        inline uint16_t hton16(uint16_t value)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
#else
            return value;
#endif
        }

        /**
         * @brief Convert 32-bit host to network byte order (big-endian)
         * @param value Value in host byte order
         * @return Value in network byte order (big-endian)
         */
        inline uint32_t hton32(uint32_t value)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return ((value & 0xFF000000) >> 24) |
                   ((value & 0x00FF0000) >> 8) |
                   ((value & 0x0000FF00) << 8) |
                   ((value & 0x000000FF) << 24);
#else
            return value;
#endif
        }

        /**
         * @brief Convert 64-bit host to network byte order (big-endian)
         * @param value Value in host byte order
         * @return Value in network byte order (big-endian)
         */
        inline uint64_t hton64(uint64_t value)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return ((value & 0xFF00000000000000ULL) >> 56) |
                   ((value & 0x00FF000000000000ULL) >> 40) |
                   ((value & 0x0000FF0000000000ULL) >> 24) |
                   ((value & 0x000000FF00000000ULL) >> 8) |
                   ((value & 0x00000000FF000000ULL) << 8) |
                   ((value & 0x0000000000FF0000ULL) << 24) |
                   ((value & 0x000000000000FF00ULL) << 40) |
                   ((value & 0x00000000000000FFULL) << 56);
#else
            return value;
#endif
        }

        /**
         * @brief Convert 16-bit network to host byte order
         * @param value Value in network byte order
         * @return Value in host byte order
         */
        inline uint16_t ntoh16(uint16_t value) { return hton16(value); }

        /**
         * @brief Convert 32-bit network to host byte order
         * @param value Value in network byte order
         * @return Value in host byte order
         */
        inline uint32_t ntoh32(uint32_t value) { return hton32(value); }

        /**
         * @brief Convert 64-bit network to host byte order
         * @param value Value in network byte order
         * @return Value in host byte order
         */
        inline uint64_t ntoh64(uint64_t value) { return hton64(value); }

        /**
         * @brief Convert float to 32-bit integer representation
         *
         * Type-punning via memcpy for safe bit-level access without
         * violating strict aliasing rules.
         *
         * @param value Floating-point value
         * @return 32-bit integer with same bit pattern
         */
        inline uint32_t floatToBits(float value)
        {
            uint32_t bits;
            std::memcpy(&bits, &value, sizeof(float));
            return bits;
        }

        /**
         * @brief Convert 32-bit integer to float representation
         * @param bits 32-bit integer bit pattern
         * @return Floating-point value with same bit pattern
         */
        inline float bitsToFloat(uint32_t bits)
        {
            float value;
            std::memcpy(&value, &bits, sizeof(float));
            return value;
        }

        /**
         * @brief Convert double to 64-bit integer representation
         * @param value Double-precision floating-point value
         * @return 64-bit integer with same bit pattern
         */
        inline uint64_t doubleToBits(double value)
        {
            uint64_t bits;
            std::memcpy(&bits, &value, sizeof(double));
            return bits;
        }

        /**
         * @brief Convert 64-bit integer to double representation
         * @param bits 64-bit integer bit pattern
         * @return Double-precision floating-point value with same bit pattern
         */
        inline double bitsToDouble(uint64_t bits)
        {
            double value;
            std::memcpy(&value, &bits, sizeof(double));
            return value;
        }

    } // namespace utils
} // namespace limp
