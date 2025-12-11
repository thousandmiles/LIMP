#pragma once

#include <cstdint>
#include <cstring>

namespace limp
{
    namespace utils
    {

        // Endianness conversion functions
        inline uint16_t hton16(uint16_t value)
        {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
#else
            return value;
#endif
        }

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

        inline uint16_t ntoh16(uint16_t value) { return hton16(value); }
        inline uint32_t ntoh32(uint32_t value) { return hton32(value); }
        inline uint64_t ntoh64(uint64_t value) { return hton64(value); }

        // Float conversion helpers
        inline uint32_t floatToBits(float value)
        {
            uint32_t bits;
            std::memcpy(&bits, &value, sizeof(float));
            return bits;
        }

        inline float bitsToFloat(uint32_t bits)
        {
            float value;
            std::memcpy(&value, &bits, sizeof(float));
            return value;
        }

        inline uint64_t doubleToBits(double value)
        {
            uint64_t bits;
            std::memcpy(&bits, &value, sizeof(double));
            return bits;
        }

        inline double bitsToDouble(uint64_t bits)
        {
            double value;
            std::memcpy(&value, &bits, sizeof(double));
            return value;
        }

    } // namespace utils
} // namespace limp
