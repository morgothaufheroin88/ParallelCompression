//
// Created by cx9ps3 on 04.11.2024.
//

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#if defined(SSE42_SUPPORTED) && (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86))
#define PARALLEL_COMPRESSION_X86_CRC32 1
#include <smmintrin.h>
#else
#define PARALLEL_COMPRESSION_X86_CRC32 0
#endif

#if (defined(__aarch64__) || defined(__arm__) || defined(_M_ARM64)) && defined(__ARM_FEATURE_CRC32)
#define PARALLEL_COMPRESSION_ARM_CRC32 1
#include <arm_acle.h>
#else
#define PARALLEL_COMPRESSION_ARM_CRC32 0
#endif

namespace deflate
{
#if PARALLEL_COMPRESSION_X86_CRC32 && (defined(__clang__) || defined(__GNUC__))
    __attribute__((target("sse4.2")))
#endif

    inline std::uint32_t crc32(const std::vector<std::byte> &data)
    {
        std::uint32_t crc = 0xFF'FF'FF'FFU;

#if PARALLEL_COMPRESSION_X86_CRC32
        for(const auto byte : data)
        {
             crc = _mm_crc32_u8(crc, static_cast<std::uint8_t>(byte));
        }
        return crc ^ 0xFF'FF'FF'FFU;
#elif PARALLEL_COMPRESSION_ARM_CRC32
        for (const auto byte: data)
        {
            crc = __crc32cb(crc, static_cast<std::uint8_t>(byte));
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
