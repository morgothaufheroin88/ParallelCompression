//
// Created by cx9ps3 on 20.08.2024.
//

#pragma once
#include "BitBuffer.hpp"
#include "HuffmanTree.hpp"
#include "LZ77.hpp"


namespace deflate
{
    class DynamicHuffmanEncoder
    {
    private:
        void createShortSequence(const std::vector<std::uint8_t> &codeLengths);
        void createRLECodes(std::uint8_t length, std::int16_t count);
        void encodeCodeLengths(const std::vector<std::uint8_t> &lengths);
        BitBuffer bitBuffer;
        std::vector<std::int16_t> shortSequence;
        std::vector<std::pair<std::uint8_t, std::uint8_t>> shortSequenceExtraBits;

    public:
        std::vector<std::byte> encodeData(const std::vector<LZ77::Match> &lz77Matches, bool isLastBlock);
    };
}// namespace deflate
