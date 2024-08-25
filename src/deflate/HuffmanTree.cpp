//
// Created by cx9ps3 on 19.08.2024.
//

#include "HuffmanTree.hpp"
#include <algorithm>
#include <array>
#include <bitset>
#include <iostream>
#include <stack>

std::vector<std::uint32_t> deflate::HuffmanTree::countFrequencies(const std::vector<std::int16_t> &symbols, const std::size_t alphabetSize) const
{
    std::vector<std::uint32_t> frequencies(static_cast<std::vector<std::uint32_t>::value_type>(alphabetSize), 0);
    for (const auto symbol: symbols)
    {
        ++frequencies[static_cast<uint16_t>(symbol)];
    }

    return frequencies;
}

void deflate::HuffmanTree::createNodes(const std::vector<std::uint32_t> &frequencies)
{
    const auto countOfFrequencies = static_cast<std::uint16_t>(frequencies.size());
    for (std::uint16_t i = 0; i < countOfFrequencies; ++i)
    {
        if (frequencies[i] > 0)
        {
            Node node;
            node.frequency = frequencies[i];
            node.symbol = static_cast<std::int16_t>(i);
            treeNodes.push_back(node);
            minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));
        }
    }
}

void deflate::HuffmanTree::buildTree()
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

        const auto parentIndex = treeNodes.size() - 1;
        treeNodes[left].parentId = static_cast<std::uint32_t>(parentIndex);
        treeNodes[right].parentId = static_cast<std::uint32_t>(parentIndex);

        minimalHeap.push(static_cast<std::uint32_t>(parentIndex));
    }
}

void deflate::HuffmanTree::calculateCodesLengths(std::int32_t rootIndex)
{
    if (rootIndex == -1)
    {
        return;
    }

    std::stack<std::pair<std::uint32_t, std::uint8_t>> stack;
    stack.emplace(rootIndex, 0);

    while (!stack.empty())
    {
        const auto [nodesIndex, currentLength] = stack.top();
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

std::vector<std::uint8_t> deflate::HuffmanTree::getLengthsFromNodes(const std::uint16_t size) const
{

    std::vector<std::uint8_t> lengths(size, 0);
    if (treeNodes.size() == 1)
    {
        lengths[static_cast<std::uint16_t>(treeNodes[0].symbol)] = 1;
        while (lengths.back() == 0)
        {
            lengths.pop_back();
        }
        return lengths;
    }

    for (const auto &node: treeNodes)
    {
        if (node.symbol > -1)
        {
            lengths[static_cast<std::uint16_t>(node.symbol)] = node.codeLength;
        }
    }

    while (lengths.back() == 0)
    {
        lengths.pop_back();
    }

    return lengths;
}

deflate::HuffmanTree::HuffmanTree(const std::vector<std::int16_t> &symbols, const std::size_t alphabetSize)
{
    const auto frequencies = countFrequencies(symbols, alphabetSize);
    createNodes(frequencies);
    buildTree();
    calculateCodesLengths(static_cast<std::int32_t>(treeNodes.size() - 1));
    std::ranges::sort(treeNodes, NodeSortCompare());
}

deflate::CodeTable::DynamicCodeTable deflate::CodeTable::createCodeTable(const std::vector<std::uint8_t> &codeLengths, const std::uint16_t codeTableSize)
{
    if (codeLengths.size() == 1)
    {
        DynamicCodeTable codeTable;
        CanonicalHuffmanCode code;
        code.code = 1;
        code.length = 1;
        codeTable[0] = code;
        return codeTable;
    }

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
            CanonicalHuffmanCode canonicalHuffmanCode;
            canonicalHuffmanCode.code = nextCode[length];
            canonicalHuffmanCode.length = length;
            codeTable[symbol] = canonicalHuffmanCode;
            auto &value = nextCode[length];
            ++value;
        }
        ++symbol;
    }

    return codeTable;
}

deflate::CodeTable::ReverseDynamicCodeTable deflate::CodeTable::createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths, const std::uint16_t codeTableSize)
{
    const auto codeTable = createCodeTable(codeLengths, codeTableSize);
    ReverseDynamicCodeTable reverseCodeTable(codeLengths.size());

    for (const auto &[symbol, code]: codeTable)
    {
        reverseCodeTable[code] = symbol;
    }
    return reverseCodeTable;
}