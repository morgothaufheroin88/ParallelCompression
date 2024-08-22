//
// Created by cx9ps3 on 20.08.2024.
//

#include "DynamicHuffmanEncoder.hpp"

#include "BitBuffer.hpp"
#include "FixedHuffmanEncoder.hpp"
#include <algorithm>

void deflate::DynamicHuffmanEncoder::createShortSequence(const std::vector<std::uint8_t> &codeLengths)
{
    std::vector<std::uint8_t> cl;
    std::ranges::copy(codeLengths, std::back_inserter(cl));

    const auto uniquesRange = std::ranges::unique(cl);
    cl.erase(uniquesRange.begin(), uniquesRange.end());

    shortSequence.reserve(cl.size());

    auto findRange = [](const auto &rangeBegin, const auto &rangeEnd, auto target)
    {
        if (auto first = std::ranges::find(rangeBegin, rangeEnd, target); first != rangeEnd)
        {
            auto last = std::ranges::find_if_not(first, rangeEnd, [target](const std::int32_t n)
                                                 { return n == target; });
            return std::make_pair(first, last);
        }
        return std::make_pair(rangeEnd, rangeEnd);
    };

    std::int64_t offset{0};
    const auto codeLengthsBegin = codeLengths.begin();

    for (const auto length: cl)
    {
        const auto [first, last] = findRange(codeLengthsBegin + offset, codeLengths.end(), length);
        auto lengthCount = std::ranges::count(first, last, length);
        offset += lengthCount;

        //encode length with run-length encoding described in RFC1951
        //see https://datatracker.ietf.org/doc/html/rfc1951#page-13

        if (lengthCount >= 3)
        {
            --lengthCount;
        }

        createRLECodes(length, static_cast<std::int16_t>(lengthCount));
    }
}
void deflate::DynamicHuffmanEncoder::createRLECodes(const std::uint8_t length, std::int16_t count)
{
    auto pushRemainingLengths = [count](std::vector<std::int16_t> &codes, const std::uint8_t value)
    {
        for (std::int16_t i = 0; i < count; ++i)
        {
            codes.push_back(value);
        }
    };

    if (length == 0)
    {
        if ((count >= 3) && (count <= 10))
        {
            shortSequence.push_back(0);
            shortSequence.push_back(17);
            shortSequenceExtraBits.emplace_back(static_cast<std::uint8_t>(count - 3), 3);
        }
        else if (count > 10)
        {
            shortSequence.push_back(0);
            shortSequence.push_back(18);
            shortSequenceExtraBits.emplace_back(static_cast<std::uint8_t>(count - 11), 7);
        }
        else
        {
            pushRemainingLengths(shortSequence, length);
        }
    }
    else if ((count >= 3) && (count <= 6))
    {
        shortSequence.push_back(length);
        shortSequence.push_back(16);
        shortSequenceExtraBits.emplace_back(static_cast<std::uint8_t>(count - 3), 2);
    }
    else if (count < 3)
    {
        pushRemainingLengths(shortSequence, length);
    }
    else if (count > 6)
    {
        while (count > 0)
        {
            const auto repeatCount = std::min(count, static_cast<std::int16_t>(6));
            if (repeatCount > 2)
            {
                shortSequence.push_back(length);
                shortSequence.push_back(16);
                shortSequenceExtraBits.emplace_back(static_cast<std::uint8_t>(count - 3), 2);
            }
            else
            {
                pushRemainingLengths(shortSequence, length);
            }
            count -= repeatCount;
        }
    }
}

void deflate::DynamicHuffmanEncoder::encodeCodeLengths(const std::vector<std::uint8_t> &lengths)
{
    createShortSequence(lengths);

    const HuffmanTree huffmanTree(shortSequence, 19);
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

    std::uint16_t extraBitsIndex{0};
    for (const auto length: shortSequence)
    {
        const auto [code, codeLength] = codeTable[static_cast<std::uint16_t>(length)];
        bitBuffer.writeBits(static_cast<std::uint32_t>(code), codeLength);

        if ((length == 16) || (length == 17) || (length == 18))
        {
            const auto [extraBits, extraBitsLength] = shortSequenceExtraBits[extraBitsIndex];
            bitBuffer.writeBits(static_cast<std::uint32_t>(extraBits), extraBitsLength);
            ++extraBitsIndex;
        }
    }
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
            distances.push_back(distanceCode.index);
        }
        else
        {
            literalsAndLengths.push_back(static_cast<std::int16_t>(match.literal));
        }
    }

    const HuffmanTree literalsAndLengthsTree(literalsAndLengths, FixedHuffmanEncoder::LITERALS_AND_DISTANCES_ALPHABET_SIZE);
    const HuffmanTree distancesTree(distances, FixedHuffmanEncoder::DISTANCES_ALPHABET_SIZE);

    const auto literalsCodeLengths = literalsAndLengthsTree.getLengthsFromNodes(FixedHuffmanEncoder::LITERALS_AND_DISTANCES_ALPHABET_SIZE);
    auto distancesCodeLengths = distancesTree.getLengthsFromNodes(FixedHuffmanEncoder::DISTANCES_ALPHABET_SIZE);

    std::ranges::copy(distancesCodeLengths, std::back_inserter(literalsAndLengths));


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
    bitBuffer.writeBits(static_cast<std::uint32_t>(literalsAndLengths.size() - 257), 5);

    //write HDIST
    bitBuffer.writeBits(static_cast<std::uint32_t>(distancesCodeLengths.size() - 1), 5);

    //write encoded lengths
    encodeCodeLengths(literalsCodeLengths);

    return bitBuffer.getBytes();
}