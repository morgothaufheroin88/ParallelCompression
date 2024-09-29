//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include <cstdint>
#include <vector>

namespace deflate
{
    class LZ77
    {
    public:
        struct Match
        {
            std::byte literal;
            std::uint16_t distance;
            std::uint16_t length;
        };

    private:
        constexpr static std::uint16_t MAX_MATCH_LENGTH = 258;
        static constexpr std::uint16_t WINDOW_SIZE = 32 * 1024;
        [[nodiscard]] static Match findBestMatch(const std::vector<std::byte> &data, std::uint16_t position);

    public:
        static std::vector<Match> compress(const std::vector<std::byte> &dataToCompress);
        static std::vector<std::byte> decompress(const std::vector<Match> &compressedData);
    };
}// namespace deflate
