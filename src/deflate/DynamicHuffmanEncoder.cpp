//
// Created by cx9ps3 on 20.08.2024.
//

#include "DynamicHuffmanEncoder.hpp"

#include "BitBuffer.hpp"
#include "FixedHuffmanEncoder.hpp"
#include <algorithm>
#include <bitset>
#include <iostream>

std::uint32_t deflate::DynamicHuffmanEncoder::reverseBits(const std::uint32_t bits, const std::uint8_t bitsCount) const
{
    std::uint16_t reversedCode = 0;
    for (std::uint8_t j = 0; j < bitsCount; ++j)
    {
        reversedCode |= ((bits >> j) & 1U) << (bitsCount - 1U - j);
    }
    return reversedCode;
}

void deflate::DynamicHuffmanEncoder::encodeCodeLengths(const std::vector<std::uint8_t> &lengths)
{
    std::vector<std::int16_t> sq;
    auto it = lengths.begin();
    while (it != lengths.end())
    {
        std::uint32_t extraBits = 0;
        std::uint32_t extraBitsLength = 0;

        const auto code = encodeCodeLength(lengths, it, extraBits, extraBitsLength);
        sq.push_back(static_cast<std::int16_t>(code));
    }

    const HuffmanTree huffmanTree(sq, 19);
    const auto ccl = huffmanTree.getLengthsFromNodes(19);

    constexpr std::array<std::uint8_t, 19> permuteOrder = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    std::vector<std::uint8_t> permutedCCL(19, 0);

    //permute ccl
    for (std::size_t i = 0; i < 19; ++i)
    {
        if (permuteOrder[i] < ccl.size())
        {
            permutedCCL[i] = ccl[permuteOrder[i]];
        }
    }

    //remove trailing zeros
    while (permutedCCL.back() == 0)
    {
        permutedCCL.pop_back();
    }

    //write HCLEN
    bitBuffer.writeBits(static_cast<std::uint32_t>(permutedCCL.size() - 4), 4);

    //write CLL
    for (const auto length: permutedCCL)
    {
        bitBuffer.writeBits(static_cast<std::uint32_t>(length), 3);
    }

    auto codeTable = CodeTable::createCodeTable(ccl, 19);

    it = lengths.begin();
    while (it != lengths.end())
    {
        std::uint32_t extraBits = 0;
        std::uint32_t extraBitsLength = 0;
        const auto code = encodeCodeLength(lengths, it, extraBits, extraBitsLength);
        const auto [tableCode, tableCodeLength] = codeTable[static_cast<std::uint16_t>(code)];

        bitBuffer.writeBits(reverseBits(tableCode, tableCodeLength), tableCodeLength);
        bitBuffer.writeBits(extraBits, extraBitsLength);
    }
}

void deflate::DynamicHuffmanEncoder::encodeLZ77Matches(const std::vector<LZ77::Match> &lz77Matches)
{
    auto FIXED_LENGTHS_CODES = FixedHuffmanEncoder::initializeFixedCodesForLengths();
    auto FIXED_DISTANCES_CODES = FixedHuffmanEncoder::initializeFixedCodesForDistances();

    auto literalsCodeTable = CodeTable::createCodeTable(literalsCodeLengths, FixedHuffmanEncoder::LITERALS_AND_DISTANCES_ALPHABET_SIZE);
    auto distancesCodeTable = CodeTable::createCodeTable(distancesCodeLengths, FixedHuffmanEncoder::DISTANCES_ALPHABET_SIZE);

    for (const auto &match: lz77Matches)
    {
        if (match.length > 1)
        {
            const auto &lengthFixedCode = FIXED_LENGTHS_CODES[match.length];
            const auto &distanceFixedCode = FIXED_DISTANCES_CODES[match.distance];

            const auto &lengthCode = literalsCodeTable[lengthFixedCode.index + 255];
            const auto &distanceCode = distancesCodeTable[distanceFixedCode.index + 1];

            //encode length with extra bits if exists
            bitBuffer.writeBits(reverseBits(lengthCode.code, lengthCode.length), lengthCode.length);
            if (lengthFixedCode.extraBitsCount > 0)
            {
                bitBuffer.writeBits(lengthFixedCode.extraBits, lengthFixedCode.extraBitsCount);
            }

            //encode distance with extra bits if exists
            bitBuffer.writeBits(reverseBits(distanceCode.code, distanceCode.length), distanceCode.length);
            if (distanceFixedCode.extraBitsCount > 0)
            {
                bitBuffer.writeBits(distanceFixedCode.extraBits, distanceFixedCode.extraBitsCount);
            }
        }
        else
        {
            const auto &literalsCode = literalsCodeTable[static_cast<std::uint16_t>(match.literal)];
            bitBuffer.writeBits(reverseBits(literalsCode.code, literalsCode.length), literalsCode.length);
        }
    }

    //write end of block
    const auto endOfBlockCode = literalsCodeTable[256];
    bitBuffer.writeBits(reverseBits(endOfBlockCode.code, endOfBlockCode.length), endOfBlockCode.length);
}

std::vector<std::byte> deflate::DynamicHuffmanEncoder::encodeData(const std::vector<LZ77::Match> &lz77Matches, const bool isLastBlock)
{
    std::vector<std::int16_t> literalsAndLengths;
    std::vector<std::int16_t> distances;

    auto FIXED_LENGTHS_CODES = FixedHuffmanEncoder::initializeFixedCodesForLengths();
    auto FIXED_DISTANCES_CODES = FixedHuffmanEncoder::initializeFixedCodesForDistances();

    for (const auto &match: lz77Matches)
    {
        if (match.length > 1)
        {
            const auto &lengthCode = FIXED_LENGTHS_CODES[match.length];
            const auto &distanceCode = FIXED_DISTANCES_CODES[match.distance];

            literalsAndLengths.push_back(lengthCode.index + 255);
            distances.push_back(distanceCode.index + 1);
        }
        else
        {
            literalsAndLengths.push_back(static_cast<std::int16_t>(match.literal));
        }
    }
    literalsAndLengths.push_back(256);

    const HuffmanTree literalsAndLengthsTree(literalsAndLengths, FixedHuffmanEncoder::LITERALS_AND_DISTANCES_ALPHABET_SIZE);
    const HuffmanTree distancesTree(distances, FixedHuffmanEncoder::DISTANCES_ALPHABET_SIZE);

    literalsCodeLengths = literalsAndLengthsTree.getLengthsFromNodes(FixedHuffmanEncoder::LITERALS_AND_DISTANCES_ALPHABET_SIZE);
    distancesCodeLengths = distancesTree.getLengthsFromNodes(FixedHuffmanEncoder::DISTANCES_ALPHABET_SIZE);

    //write header
    if (isLastBlock)
    {
        bitBuffer.writeBits(1, 1);
    }
    else
    {
        bitBuffer.writeBits(1, 0);
    }

    bitBuffer.writeBits(0b10, 2);

    //write HLIT
    bitBuffer.writeBits(static_cast<std::uint32_t>(literalsCodeLengths.size() - 257), 5);

    //write HDIST
    bitBuffer.writeBits(static_cast<std::uint32_t>(distancesCodeLengths.size() - 1), 5);

    std::vector<std::uint8_t> literalsAndDistancesCodesLengths;

    std::ranges::copy(literalsCodeLengths, std::back_inserter(literalsAndDistancesCodesLengths));
    std::ranges::copy(distancesCodeLengths, std::back_inserter(literalsAndDistancesCodesLengths));

    //write encoded lengths
    encodeCodeLengths(literalsAndDistancesCodesLengths);

    //write data
    encodeLZ77Matches(lz77Matches);

    return bitBuffer.getBytes();
}