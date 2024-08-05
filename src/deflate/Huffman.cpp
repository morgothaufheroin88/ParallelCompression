//
// Created by cx9ps3 on 01.08.2024.
//

#include "Huffman.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <iostream>
#include <queue>
#include <stack>

std::pair<std::vector<std::uint16_t>, std::vector<uint16_t>> deflate::Huffman::lz77MatchesToVectors(const std::vector<LZ77::Match> &lz77Matches)
{
    std::vector<std::uint16_t> literalsAndLengths;
    literalsAndLengths.reserve(lz77Matches.size());

    std::vector<std::uint16_t> distances;
    distances.reserve(lz77Matches.size());

    for (const auto &match: lz77Matches)
    {
        if (match.length > 1)
        {
            literalsAndLengths.push_back(match.length);
            distances.push_back(match.distance);
        }
        else
        {
            literalsAndLengths.push_back(static_cast<std::uint16_t>(match.literal));
        }
    }

    return {literalsAndLengths, distances};
}

std::vector<std::uint32_t> deflate::Huffman::countFrequencies(const std::vector<std::uint16_t> &data, std::uint16_t alphabetSize)
{
    std::vector<std::uint32_t> frequencies(alphabetSize, 0);

    for (const auto &symbol: data)
    {
        frequencies[symbol]++;
    }
    return frequencies;
}
deflate::Huffman::TreeNodes deflate::Huffman::buildTree(const std::vector<std::uint32_t> &frequencies)
{
    std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, NodeCompare> minimalHeap;
    TreeNodes treeNodes;
    treeNodes.reserve(frequencies.size());

    for (std::int16_t i = 0; i < frequencies.size(); i++)
    {
        if (frequencies[i] > 0)
        {
            Node node;
            node.frequency = frequencies[i];
            node.symbol = i;
            treeNodes.push_back(node);
            minimalHeap.push(treeNodes.size() - 1);
        }
    }

    while (minimalHeap.size() > 1)
    {
        auto left = minimalHeap.top();
        minimalHeap.pop();

        auto right = minimalHeap.top();
        minimalHeap.pop();

        auto newFrequency = treeNodes[left].frequency + treeNodes[right].frequency;
        Node node;
        node.frequency = newFrequency;
        node.leftChildId = left;
        node.rightChildId = right;
        treeNodes.push_back(node);

        auto parentIndex = treeNodes.size() - 1;
        treeNodes[left].parentId = parentIndex;
        treeNodes[right].parentId = parentIndex;

        minimalHeap.push(parentIndex);
    }

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
        if (node.leftChildId == -1 && node.rightChildId == -1)
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
    auto [literalsAndLengths, distances] = lz77MatchesToVectors(lz77CompressedData);

    auto frequencies = countFrequencies(literalsAndLengths, LITERALS_AND_LENGTHS_ALPHABET_SIZE);
    auto treeNodes = buildTree(frequencies);
    calculateCodesLengths(treeNodes, treeNodes.size() - 1);

    std::ranges::sort(treeNodes, NodeSort());
    std::vector<std::uint8_t> codeLengths = {3, 3, 3, 3, 3, 2, 4, 4};
    auto codeTable = createCodeTable(codeLengths);


    return compressedData;
}

std::unordered_map<std::uint16_t, std::uint16_t> deflate::Huffman::createCodeTable(const std::vector<std::uint8_t> &codeLengths)
{
    std::unordered_map<std::uint16_t, std::uint16_t> codeTable;
    codeTable.reserve(codeLengths.size());

    std::array<std::uint16_t, MAX_BITS + 1> codeLengthsCount = {0};
    std::array<std::uint16_t, MAX_BITS + 1> nextCode = {0};

    for (const auto &length: codeLengths)
    {
        codeLengthsCount[length]++;
    }

    std::uint16_t code{0};
    for (std::uint8_t bit = 1; bit <= MAX_BITS; bit++)
    {
        code = (code + codeLengthsCount[bit - 1]) << 1;
        nextCode[bit] = code;
    }


    for (std::uint32_t i = 0; i < codeLengths.size(); i++)
    {
        auto length = codeLengths[i];
        if (length != 0)
        {
            codeTable[nextCode[length]] = 0;
            nextCode[length]++;
        }
    }

    return codeTable;
}

std::unordered_map<std::uint16_t, std::uint16_t> deflate::Huffman::createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths)
{
    auto codeTable = createCodeTable(codeLengths);
    std::unordered_map<std::uint16_t, std::uint16_t> reverseCodeTable;
    reverseCodeTable.reserve(codeLengths.size());

    for (const auto &[code, symbol]: codeTable)
    {
        reverseCodeTable[code] = symbol;
    }
    return reverseCodeTable;
}

std::vector<std::byte> deflate::Huffman::encodeWithFixedCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock)
{
    std::vector<std::byte> compressedData;
    compressedData.reserve(lz77CompressedData.size());

    //write header
    std::byte byteToWrite{0};
    if (isLastBlock)
    {
        byteToWrite |= std::byte(0b1);
    }

    byteToWrite |= (std::byte{0b01} << 2);

    for (int i = 0; i < 8; i++)
    {
        std::uint8_t mask = (1 << i + 1) - 1;
        std::cout << std::bitset<8>(static_cast<char>(mask)).to_string() << "\n";
    }


    for (const auto &lz77Match: lz77CompressedData)
    {
        if (lz77Match.length > 1)
        {
            const auto &lengthCode = FIXED_LENGTHS_CODES[lz77Match.length];
            const auto &distanceCode = FIXED_DISTANCES_CODES[lz77Match.distance];
            std::cout << "Code : " << std::bitset<8>(static_cast<char>(distanceCode.code)).to_string() << " ,distance:  " << lz77Match.distance << "=" << distanceCode.distance << " extra bits: " << std::bitset<8>(distanceCode.extraBits).to_string() << "\n";
            std::cout << "Code : " << std::bitset<8>(static_cast<char>(lengthCode.code)).to_string() << " ,length:  " << lz77Match.length << "=" << static_cast<int>(lengthCode.length) << " extra bits: " << std::bitset<8>(lengthCode.extraBits).to_string() << " extra bits count : " << static_cast<int>(lengthCode.extraBitsCount) << "\n";
        }
    }

    return compressedData;
}