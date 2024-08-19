//
// Created by cx9ps3 on 01.08.2024.
//

#include "Huffman.hpp"
#include <algorithm>
#include <bitset>
#include <iostream>
#include <ranges>
#include <stack>

std::vector<std::byte> deflate::Huffman::encodeWithDynamicCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock)
{
    std::vector<std::byte> compressedData;
    compressedData.reserve(lz77CompressedData.size());

    const auto [literalsFrequencies, lengthsFrequencies, distancesFrequencies] = countFrequencies(lz77CompressedData);

    auto literalsCodeTable = createCodeTableForLiterals(literalsFrequencies, lengthsFrequencies);

    auto distancesCodeTable = createCodeTableForDistances(distancesFrequencies);

    return compressedData;
}

deflate::Huffman::Frequencies deflate::Huffman::countFrequencies(const std::vector<LZ77::Match> &lz77Matches)
{
    std::vector<std::uint32_t> literalsFrequencies(MAX_LITERAL, 0);
    std::vector<uint32_t> lengthsFrequencies(LITERALS_AND_DISTANCES_ALPHABET_SIZE - MAX_LITERAL, 0);
    std::vector<uint32_t> distancesFrequencies(DISTANCES_ALPHABET_SIZE, 0);

    for (const auto &match: lz77Matches)
    {
        if (match.length > 1)
        {
            const auto &lengthCode = FIXED_LENGTHS_CODES[match.length];
            const auto &distanceCode = FIXED_DISTANCES_CODES[match.distance];

            lengthsFrequencies[lengthCode.index]++;
            distancesFrequencies[distanceCode.index]++;
        }
        else
        {
            literalsFrequencies[static_cast<std::uint8_t>(match.literal)]++;
        }
    }

    Frequencies frequencies;
    std::get<0>(frequencies) = literalsFrequencies;
    std::get<1>(frequencies) = lengthsFrequencies;
    std::get<2>(frequencies) = distancesFrequencies;

    return frequencies;
}

std::vector<std::uint16_t> deflate::Huffman::createShortSequence(const std::vector<std::uint8_t> &codeLengths)
{
    std::vector<std::uint8_t> cl;
    std::ranges::copy(codeLengths, std::back_inserter(cl));

    auto uniquesRange = std::ranges::unique(cl);
    cl.erase(uniquesRange.begin(), uniquesRange.end());

    std::vector<std::uint16_t> rleCodes;
    rleCodes.reserve(cl.size());

    auto findRange = [](const auto &rangeBegin, const auto &rangeEnd, auto target)
    {
        if (auto first = std::ranges::find(rangeBegin, rangeEnd, target); first != rangeEnd)
        {
            auto last = std::ranges::find_if_not(first, rangeEnd, [target](int n)
                                                 { return n == target; });
            return std::make_pair(first, last);
        }
        return std::make_pair(rangeEnd, rangeEnd);
    };

    std::int64_t offset{0};
    auto codeLengthsBegin = codeLengths.begin();

    for (auto length: cl)
    {
        auto [first, last] = findRange(codeLengthsBegin + offset, codeLengths.end(), length);
        auto lengthCount = std::ranges::count(first, last, length);
        offset += lengthCount;

        //encode length with run-length encoding described in RFC1951
        //see https://datatracker.ietf.org/doc/html/rfc1951#page-13

        if (lengthCount >= 3)
        {
            --lengthCount;
        }

        createRLECodes(rleCodes, length, static_cast<std::int16_t>(lengthCount));
    }

    return rleCodes;
}

void deflate::Huffman::createRLECodes(std::vector<std::uint16_t> &rleCodes, std::uint8_t length, std::int16_t count)
{
    if (length == 0)
    {
        if (count >= 3 && count <= 10)
        {
            rleCodes.push_back(0);
            rleCodes.push_back(17);
            rleCodes.push_back(count - 3);
        }
        else if (count > 10)
        {
            rleCodes.push_back(0);
            rleCodes.push_back(18);
            rleCodes.push_back(count - 11);
        }
        else
        {
            for (int i = 0; i < count; i++)
            {
                rleCodes.push_back(length);
            }
        }
    }
    else if (count >= 3 && count <= 6)
    {
        rleCodes.push_back(length);
        rleCodes.push_back(16);
        rleCodes.push_back(count - 3);
    }
    else if (count < 3)
    {
        for (int i = 0; i < count; i++)
        {
            rleCodes.push_back(length);
        }
    }
    else if (count > 6)
    {
        while (count > 0)
        {
            const auto repeatCount = std::min(count, static_cast<std::int16_t>(6));
            if (repeatCount > 2)
            {
                rleCodes.push_back(length);
                rleCodes.push_back(16);
                rleCodes.push_back(count - 3);
            }
            else
            {
                for (std::uint32_t i = 0; i < count; ++i)
                {
                    rleCodes.push_back(length);
                }
            }
            count -= repeatCount;
        }
    }
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createCCL(const std::vector<std::int16_t> &rleCodes)
{
    std::unordered_map<std::int16_t,std::int16_t> frequencies;

    MinimalHeap minimalHeap;
    TreeNodes treeNodes;

    for (auto rleCode: rleCodes)
    {
        auto& value = frequencies[rleCode];
        ++value;
    }

    for(auto [key,value]: frequencies)
    {
        Node node;
        node.frequency = value;
        node.symbol = key;

        treeNodes.push_back(node);
        minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));

    }

    buildTree(minimalHeap, treeNodes);
    calculateCodesLengths(treeNodes, static_cast<std::uint32_t>(treeNodes.size() - 1));
    const auto ccl = getLengthsFromNodes(treeNodes,treeNodes.size());
    return createCodeTable(ccl,19);
}
