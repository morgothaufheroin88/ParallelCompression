//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include "LZ77.hpp"
#include <array>
#include <optional>
#include <queue>
#include <unordered_map>

namespace deflate
{
    class Huffman
    {
    private:


        static std::vector<std::uint16_t> createShortSequence(const std::vector<std::uint8_t> &codeLengths);
        static void createRLECodes(std::vector<std::uint16_t> &rleCodes, std::uint8_t length, std::int16_t count);
        static DynamicCodeTable createCCL(const std::vector<std::int16_t> &rleCodes);
    public:
        static std::vector<std::byte> encodeWithDynamicCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock);
        static std::vector<LZ77::Match> decodeWithFixedCodes(const std::vector<std::byte> &bitsCount);
    };
}// namespace deflate
