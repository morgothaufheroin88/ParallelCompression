//
// Created by cx9ps3 on 18.08.2024.
//

#include "../buffer/BitBuffer.hpp"
#include "FixedHuffmanEncoder.hpp"
#include <cstddef>

std::vector<std::byte> deflate::FixedHuffmanEncoder::encodeData(const std::vector<LZ77::Match> &dataToEncode, const bool isLastBlock)
{
    assert(!dataToEncode.empty(),"LZ77 matches is empty!");
    constexpr auto FIXED_LITERALS_CODES{initializeFixedCodesForLiterals()};
    constexpr auto FIXED_DISTANCES_CODES{initializeFixedCodesForDistances()};
    constexpr auto FIXED_LENGTHS_CODES{initializeFixedCodesForLengths()};
    std::vector<std::byte> compressedData;
    compressedData.reserve(dataToEncode.size());
    BitBuffer bitBuffer;

    //write header
    if (isLastBlock)
    {
        bitBuffer.writeBits(1, 1);
    }
    else
    {
        bitBuffer.writeBits(1, 0);
    }

    bitBuffer.writeBits(0b01, 2);


    for (const auto &[literal, distance, length]: dataToEncode)
    {
        if (length > 1)
        {
            const auto &lengthCode = FIXED_LENGTHS_CODES[length];
            const auto &distanceCode = FIXED_DISTANCES_CODES[distance];

            //encode lz77 lengths
            bitBuffer.writeBits(lengthCode.code, lengthCode.codeLength);
            bitBuffer.writeBits(lengthCode.extraBits, lengthCode.extraBitsCount);

            //encode lz77 distance
            bitBuffer.writeBits(distanceCode.code, 5);
            bitBuffer.writeBits(distanceCode.extraBits, distanceCode.extraBitsCount);
        }
        else
        {
            const auto &literalCode = FIXED_LITERALS_CODES[static_cast<std::uint8_t>(literal)];

            //encode lz77 literal
            bitBuffer.writeBits(literalCode.code, literalCode.codeLength);
        }
    }

    //write end of block
    bitBuffer.writeBits(0, 7);

    return bitBuffer.getBytes();
}