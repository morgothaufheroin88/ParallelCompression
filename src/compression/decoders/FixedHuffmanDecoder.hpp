//
// Created by cx9ps3 on 18.08.2024.
//

#pragma once
#include "../buffer/BitBuffer.hpp"
#include "../encoders/FixedHuffmanEncoder.hpp"
#include "../lz/LZ77.hpp"
#include "../tree/HuffmanTree.hpp"
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace deflate
{
    class BitBuffer;
    class FixedHuffmanDecoder
    {
    private:
        BitBuffer bitBuffer;
        bool isNextDistance = false;
        static constexpr auto FIXED_LITERALS_CODES{FixedHuffmanEncoder::initializeFixedCodesForLiterals()};
        static constexpr auto FIXED_LENGTHS_CODES{FixedHuffmanEncoder::initializeFixedCodesForLengths()};
        static constexpr auto FIXED_DISTANCES_CODES{FixedHuffmanEncoder::initializeFixedCodesForDistances()};
        CodeTable::ReverseHuffmanCodeTable literalsCodeTable;
        CodeTable::ReverseHuffmanCodeTable distancesCodeTable;

        std::optional<std::uint16_t> tryDecodeLength(std::uint16_t lengthFixedCode);
        std::optional<std::uint16_t> tryDecodeDistance(std::uint16_t code, std::uint8_t codeBitPosition);
        void decodeHeader();

    public:
        explicit FixedHuffmanDecoder(const BitBuffer &newBitBuffer);
        std::vector<LZ77::Match> decodeData();
    };
}// namespace deflate
