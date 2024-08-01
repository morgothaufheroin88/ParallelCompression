//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include <cstdint>
#include <immintrin.h>
#include <vector>

namespace deflate
{
    class LZ77
    {
    private:
        static constexpr std::uint16_t WINDOW_SIZE = 32 * 1024;

        struct Match
        {
            std::byte literal;
            std::uint16_t distance;
            std::uint16_t length;
        };

        static Match findBestMatch(const std::vector<std::byte> &data, std::uint16_t position);

    public:
        static std::vector<Match> compress(const std::vector<std::byte> &dataToCompress);
        static std::vector<std::byte> decompress(const std::vector<Match>& compressedData);
    };
}// namespace deflate
