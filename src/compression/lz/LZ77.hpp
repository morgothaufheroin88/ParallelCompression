//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include <cstdint>
#include <unordered_map>
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
        using HashTable = std::unordered_map<std::size_t, std::uint16_t>;
        [[nodiscard]] static std::size_t hashSequence(const std::vector<std::byte> &data, std::uint16_t position);
        constexpr static std::uint16_t MAX_MATCH_LENGTH = 258;
        static constexpr std::uint16_t WINDOW_SIZE = 32 * 1024;
        [[nodiscard]] static Match findBestMatch(const std::vector<std::byte> &data, std::uint16_t position, HashTable &hashTable);
        [[nodiscard]] static Match findBestMatch(const std::vector<std::byte> &data, std::uint16_t position);

    public:
        static std::vector<Match> compress(const std::vector<std::byte> &dataToCompress, bool isUseHashMap);
        static std::vector<std::byte> decompress(const std::vector<Match> &compressedData);
        static void decompress(const std::vector<Match> &compressedData, std::vector<std::byte> &decompressedData);
    };
}// namespace deflate
