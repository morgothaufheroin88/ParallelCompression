//
// Created by cx9ps3 on 26.08.2024.
//

#pragma once
#include "../buffer/BitBuffer.hpp"
#include "../tree/HuffmanTree.hpp"
#include "../lz/LZ77.hpp"

#include <optional>
#include <vector>

namespace deflate
{
    class DynamicHuffmanDecoder
    {
    private:
        BitBuffer bitBuffer;
        void decodeHeader();
        std::vector<std::uint8_t> decodeCCL();
        void decodeCodeLengths();
        std::vector<LZ77::Match> decodeBody();
        [[nodiscard]] inline std::uint32_t reverseBits(std::uint32_t bits, std::uint8_t bitsCount) const;
        [[nodiscard]] inline std::optional<std::uint16_t> tryDecodeLength(std::uint16_t lengthFixedCode);
        [[nodiscard]] inline std::optional<std::uint16_t> tryDecodeDistance(std::uint16_t code, std::uint8_t codeBitPosition);

        std::vector<std::uint8_t> literalsCodeLengths;
        std::vector<std::uint8_t> distanceCodeLengths;
        std::uint8_t HLIT{0};
        std::uint8_t HDIST{0};
        std::uint8_t HCLEN{0};
        CodeTable::ReverseDynamicCodeTable literalsCodeTable;
        CodeTable::ReverseDynamicCodeTable distancesCodeTable;

    public:
        explicit DynamicHuffmanDecoder(const BitBuffer &newBitsBuffer);
        std::vector<LZ77::Match> decodeData();
    };
}// namespace deflate
