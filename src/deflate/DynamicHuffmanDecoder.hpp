//
// Created by cx9ps3 on 26.08.2024.
//

#pragma once
#include "BitBuffer.hpp"
#include "LZ77.hpp"

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
        [[nodiscard]] inline std::uint32_t reverseBits(std::uint32_t bits, std::uint8_t bitsCount) const;

        std::vector<std::uint8_t> literalsCodeLengths;
        std::vector<std::uint8_t> distanceCodeLengths;
        std::uint8_t HLIT{0};
        std::uint8_t HDIST{0};
        std::uint8_t HCLEN{0};

    public:
        explicit DynamicHuffmanDecoder(const BitBuffer &newBitsBuffer);
        std::vector<LZ77::Match> decodeData();
    };
}// namespace deflate
