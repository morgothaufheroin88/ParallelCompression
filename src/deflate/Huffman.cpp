//
// Created by cx9ps3 on 01.08.2024.
//

#include "Huffman.hpp"
#include <algorithm>
#include <bitset>
#include <iostream>
#include <ranges>
#include <stack>


deflate::Huffman::TreeNodes deflate::Huffman::buildLiteralsAndLengthsTree(const std::vector<std::uint32_t> &literalsFrequencies, const std::vector<std::uint32_t> &lengthsFrequencies)
{
    MinimalHeap minimalHeap;
    TreeNodes treeNodes;
    treeNodes.reserve(literalsFrequencies.size() + lengthsFrequencies.size());

    for (std::int16_t i = 0; i < static_cast<std::int16_t>(literalsFrequencies.size()); i++)
    {
        if (literalsFrequencies[i] > 0)
        {
            Node node;
            node.frequency = literalsFrequencies[i];
            node.symbol = i;
            node.nodeType = NodeType::LITERAL;
            treeNodes.push_back(node);
            minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));
        }
    }

    for (std::int16_t i = 0; i < static_cast<std::int16_t>(lengthsFrequencies.size()); i++)
    {
        if (lengthsFrequencies[i])
        {
            Node node;
            node.frequency = lengthsFrequencies[i];
            node.symbol = MAX_LITERAL + i;
            node.nodeType = NodeType::LENGTH;
            treeNodes.push_back(node);
            minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));
        }
    }

    buildTree(minimalHeap, treeNodes);
    calculateCodesLengths(treeNodes, static_cast<std::uint32_t>(treeNodes.size() - 1));
    return treeNodes;
}

void deflate::Huffman::calculateCodesLengths(deflate::Huffman::TreeNodes &treeNodes, std::uint32_t rootIndex)
{
    if (rootIndex == -1)
    {
        return;
    }

    std::stack<std::pair<std::uint32_t, std::uint8_t>> stack;
    stack.emplace(rootIndex, 0);

    while (!stack.empty())
    {
        auto [nodesIndex, currentLength] = stack.top();
        stack.pop();

        auto &node = treeNodes[nodesIndex];
        if ((node.frequency != 0) && (node.leftChildId == -1) && (node.rightChildId == -1))
        {
            node.codeLength = currentLength;
        }
        else
        {
            if (node.rightChildId != -1)
            {
                stack.emplace(node.rightChildId, currentLength + 1);
            }
            if (node.leftChildId != -1)
            {
                stack.emplace(node.leftChildId, currentLength + 1);
            }
        }
    }
}

std::vector<std::byte> deflate::Huffman::encodeWithDynamicCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock)
{
    std::vector<std::byte> compressedData;
    compressedData.reserve(lz77CompressedData.size());

    const auto [literalsFrequencies, lengthsFrequencies, distancesFrequencies] = countFrequencies(lz77CompressedData);

    auto literalsCodeTable = createCodeTableForLiterals(literalsFrequencies, lengthsFrequencies);

    auto distancesCodeTable = createCodeTableForDistances(distancesFrequencies);

    return compressedData;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize)
{
    DynamicCodeTable codeTable(codeTableSize);
    codeTable.reserve(codeLengths.size());

    std::array<std::uint16_t, MAX_BITS + 1> codeLengthsCount = {0};
    std::array<std::uint16_t, MAX_BITS + 1> nextCode = {0};

    for (const auto &length: codeLengths)
    {
        auto &value = codeLengthsCount[length];
        ++value;
    }

    std::uint16_t code{0};
    for (std::uint8_t bit = 1; bit <= MAX_BITS; ++bit)
    {
        code = static_cast<std::uint16_t>((code + codeLengthsCount[bit - 1]) << 1);
        nextCode[bit] = code;
    }

    std::uint16_t symbol{0};
    for (const auto length: codeLengths)
    {
        if (length != 0)
        {
            std::cout << symbol << " " << std::bitset<16>(nextCode[length]).to_string().substr(16 - length, 16) << "\n";

            CanonicalHuffmanCode canonicalHuffmanCode;
            canonicalHuffmanCode.code = nextCode[length];
            canonicalHuffmanCode.length = length;
            codeTable[symbol] = canonicalHuffmanCode;
            auto& value = nextCode[length];
            ++value;
        }
        ++symbol;
    }

    return codeTable;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize)
{
    auto codeTable = createCodeTable(codeLengths, codeTableSize);
    DynamicCodeTable reverseCodeTable;
    reverseCodeTable.reserve(codeLengths.size());

    for (const auto &[code, symbol]: codeTable)
    {
        reverseCodeTable[code] = symbol;
    }
    return reverseCodeTable;
}

void deflate::Huffman::addBitsToBuffer(std::vector<std::byte> &buffer, std::uint16_t value, std::uint8_t bitCount, uint8_t &bitPosition, std::byte &currentByte)
{
    while (bitCount > 0)
    {
        const auto bitsToWrite = std::min(bitCount, static_cast<std::uint8_t>(8 - bitPosition));
        const auto bits = static_cast<std::byte>((value & ((1 << bitsToWrite) - 1)) << bitPosition);
        currentByte |= bits;

        bitPosition += bitsToWrite;
        bitCount -= bitsToWrite;
        value >>= bitsToWrite;

        if (bitPosition == 8)
        {
            buffer.push_back(currentByte);
            currentByte = std::byte{0};
            bitPosition = 0;
        }
    }
}
/*
std::vector<deflate::LZ77::Match> deflate::Huffman::decodeWithFixedCodes(const std::vector<std::byte> &compressedData)
{

    std::vector<LZ77::Match> lz77Decompressed;
    lz77Decompressed.reserve(compressedData.size());

    std::uint8_t byteBitPosition{0};
    std::size_t byteIndex{0};
    std::byte currentLiteral{0};
    std::uint16_t currentLength{0};
    std::uint16_t currentDistance{0};
    std::uint8_t extraBits = 0;
    bool isNextDistance = false;

    bool isHeaderParsed = false;
    auto currentByte = compressedData[0];

    auto readBit = [&]()
    {
        auto bit = (static_cast<std::uint16_t>(currentByte) >> byteBitPosition) & 1;
        byteBitPosition++;
        if (byteBitPosition == 8)
        {
            byteIndex++;
            byteBitPosition = 0;
            currentByte = compressedData[byteIndex];
        }
        return bit;
    };



    auto tryDecodeLength = [&]()
    {

    };

    auto tryDecodeDistance = [&]()
    {

    };


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
*/

void deflate::Huffman::buildTree(std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, NodeCompare> &minimalHeap, TreeNodes &treeNodes)
{
    while (minimalHeap.size() > 1)
    {
        const auto left = minimalHeap.top();
        minimalHeap.pop();

        const auto right = minimalHeap.top();
        minimalHeap.pop();

        const auto newFrequency = treeNodes[left].frequency + treeNodes[right].frequency;
        Node node;
        node.frequency = newFrequency;
        node.leftChildId = static_cast<std::int32_t>(left);
        node.rightChildId = static_cast<std::int32_t>(right);
        treeNodes.push_back(node);

        auto parentIndex = treeNodes.size() - 1;
        treeNodes[left].parentId = static_cast<std::uint32_t>(parentIndex);
        treeNodes[right].parentId = static_cast<std::uint32_t>(parentIndex);

        minimalHeap.push(static_cast<std::int32_t>(parentIndex));
    }
}

std::vector<std::uint8_t> deflate::Huffman::getLengthsFromNodes(const deflate::Huffman::TreeNodes &treeNodes, std::uint16_t size)
{
    std::vector<std::uint8_t> lengths(size, 0);
    for (const auto &node: treeNodes)
    {
        if (node.symbol > -1)
        {
            lengths[node.symbol] = node.codeLength;
        }
    }
    return lengths;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createCodeTableForLiterals(const std::vector<uint32_t> &literalsFrequencies, const std::vector<uint32_t> &lengthsFrequencies)
{
    auto literalsAndLengthsTreeNodes = buildLiteralsAndLengthsTree(literalsFrequencies, lengthsFrequencies);

    if (literalsAndLengthsTreeNodes.size() == 1)
    {
        DynamicCodeTable codeTable(LITERALS_AND_DISTANCES_ALPHABET_SIZE);
        CanonicalHuffmanCode code;
        code.code = 1;
        code.length = 1;
        codeTable[literalsAndLengthsTreeNodes[0].symbol] = code;
        return codeTable;
    }

    std::ranges::sort(literalsAndLengthsTreeNodes, NodeSortCompare());
    const auto literalsCodesLengths = getLengthsFromNodes(literalsAndLengthsTreeNodes, LITERALS_AND_DISTANCES_ALPHABET_SIZE + 1);
    const auto shortSequence = createShortSequence(literalsCodesLengths);

    return createCodeTable(literalsCodesLengths, LITERALS_AND_DISTANCES_ALPHABET_SIZE);
}

deflate::Huffman::TreeNodes deflate::Huffman::buildDistancesTree(const std::vector<std::uint32_t> &distancesFrequencies)
{
    MinimalHeap minimalHeap;
    TreeNodes treeNodes;
    treeNodes.reserve(distancesFrequencies.size());

    for (std::int16_t i = 0; i < static_cast<std::int16_t>(distancesFrequencies.size()); i++)
    {
        if (distancesFrequencies[i] > 0)
        {
            Node node;
            node.frequency = distancesFrequencies[i];
            node.symbol = i;
            node.nodeType = NodeType::DISTANCE;
            treeNodes.push_back(node);
            minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));
        }
    }

    buildTree(minimalHeap, treeNodes);
    calculateCodesLengths(treeNodes, static_cast<std::uint32_t>(treeNodes.size() - 1));
    return treeNodes;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createCodeTableForDistances(const std::vector<uint32_t> &distancesFrequencies)
{
    auto distancesTreeNodes = buildDistancesTree(distancesFrequencies);

    if (distancesTreeNodes.size() == 1)
    {
        DynamicCodeTable codeTable(DISTANCES_ALPHABET_SIZE);
        CanonicalHuffmanCode code;
        code.code = 1;
        code.length = 1;
        codeTable[distancesTreeNodes[0].symbol] = code;
        return codeTable;
    }

    std::ranges::sort(distancesTreeNodes, NodeSortCompare());
    const auto literalsCodesLengths = getLengthsFromNodes(distancesTreeNodes, DISTANCES_ALPHABET_SIZE + 1);
    createShortSequence(literalsCodesLengths);

    return createCodeTable(literalsCodesLengths, DISTANCES_ALPHABET_SIZE);
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
