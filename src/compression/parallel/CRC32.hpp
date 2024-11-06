//
// Created by cx9ps3 on 04.11.2024.
//

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
#ifdef SSE42_SUPPORTED
#include <smmintrin.h>
#endif

namespace deflate
{
#if defined(__clang__) || defined(__GNUC__)
    __attribute__((target("crc32")))
#endif

    inline std::uint32_t crc32(const std::vector<std::byte> &data)
    {
        std::uint32_t crc = 0xFF'FF'FF'FFU;

#ifdef SSE42_SUPPORTED
        for(const auto byte : data)
        {
             crc = _mm_crc32_u8(crc, static_cast<std::uint8_t>(byte));
        }
        return crc ^ 0xFF'FF'FF'FFU;
#else


        for(const auto byte : data)
        {
            crc ^= static_cast<std::uint8_t>(byte);
            for (std::uint8_t k = 0; k < 8; ++k)
            {
                constexpr std::uint32_t POLY = 0x82'F6'3B'78U;
                crc = static_cast<bool>(crc & 1) ? ((crc >> 1) ^ POLY) : (crc >> 1);
            }
        }

        return ~crc;
#endif
    }
}// namespace deflate