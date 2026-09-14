//
// Created by cx9ps3 on 19.08.2024.
//

#include "HuffmanTree.hpp"

#include "../buffer/BitBuffer.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <format>
#include <iostream>
#include <stdexcept>
#include <stack>

std::vector<std::uint32_t> deflate::HuffmanTree::countFrequencies(const std::vector<std::int16_t> &symbols, const std::size_t alphabetSize) const
{
    std::vector<std::uint32_t> frequencies(static_cast<std::vector<std::uint32_t>::value_type>(alphabetSize), 0);
    for (const auto symbol: symbols)
    {
        assert(symbol < static_cast<std::uint16_t>(frequencies.size()), std::format("symbol {} out of range {}", symbol, frequencies.size()));
        ++frequencies[static_cast<uint16_t>(symbol)];
    }

    return frequencies;
}

void deflate::HuffmanTree::createNodes(const std::vector<std::uint32_t> &frequencies, MinimalHeap &minimalHeap)
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

void deflate::HuffmanTree::buildTree(MinimalHeap &minimalHeap)
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
        // Every node knows its depth, not only the leaves: flattening the
        // tree has to count the branches that went too deep as well.
        node.codeLength = currentLength;
        if (!((node.frequency != 0) && (node.leftChildId == -1) && (node.rightChildId == -1)))
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
        while (!lengths.empty() && lengths.back() == 0)
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

    while (!lengths.empty() && lengths.back() == 0)
    {
        lengths.pop_back();
    }

    return lengths;
}

void deflate::HuffmanTree::limitCodeLengths(const std::uint8_t maxLength)
{
    // The leaves, rarest last: those are the ones that end up deepest and
    // the ones whose codes will be lengthened if anything has to give.
    std::vector<std::uint32_t> leaves;
    for (std::uint32_t index = 0; index < treeNodes.size(); ++index)
    {
        if (treeNodes[index].symbol > -1)
        {
            leaves.push_back(index);
        }
    }
    std::ranges::sort(leaves, [this](const std::uint32_t a, const std::uint32_t b)
                      {
                          const auto &left = treeNodes[a];
                          const auto &right = treeNodes[b];
                          if (left.codeLength != right.codeLength) return left.codeLength < right.codeLength;
                          if (left.frequency != right.frequency) return left.frequency > right.frequency;
                          return left.symbol < right.symbol; });

    if (leaves.empty() || treeNodes[leaves.back()].codeLength <= maxLength)
    {
        return;
    }

    // How many codes of each length there are.  Every code deeper than the
    // limit is pulled up to it, which makes the code as a whole too big to
    // be a prefix code -- the Kraft sum goes over one -- and the way back
    // under is to push some shallower code down a level, which frees a slot
    // at the limit.  Each push pays for two nodes that went too deep, and
    // the branches below the limit count as well as the leaves, since each
    // of them is a slot the flattening did away with: zlib's gen_bitlen,
    // which counts the same way.
    std::vector<std::uint32_t> countOfLength(static_cast<std::size_t>(maxLength) + 2, 0);
    std::int32_t overflow = 0;
    for (const auto &node: treeNodes)
    {
        if (node.codeLength > maxLength)
        {
            ++overflow;
        }
    }
    for (const auto index: leaves)
    {
        ++countOfLength[std::min(treeNodes[index].codeLength, maxLength)];
    }
    while (overflow > 0)
    {
        std::uint8_t bits = maxLength - 1;
        while (countOfLength[bits] == 0)
        {
            --bits;
        }
        --countOfLength[bits];
        countOfLength[bits + 1] += 2;
        --countOfLength[maxLength];
        overflow -= 2;
    }

    // Hand the lengths back out, shortest to the commonest.
    std::size_t leaf = 0;
    for (std::uint8_t length = 1; length <= maxLength; ++length)
    {
        for (std::uint32_t count = 0; count < countOfLength[length]; ++count)
        {
            treeNodes[leaves[leaf++]].codeLength = length;
        }
    }
}

deflate::HuffmanTree::HuffmanTree(const std::vector<std::int16_t> &symbols, const std::size_t alphabetSize, const std::uint8_t maxLength)
{
    const auto frequencies = countFrequencies(symbols, alphabetSize);
    MinimalHeap minimalHeap{NodeCompare{&treeNodes}};
    createNodes(frequencies, minimalHeap);
    buildTree(minimalHeap);
    calculateCodesLengths(static_cast<std::int32_t>(treeNodes.size() - 1));
    limitCodeLengths(maxLength);
    std::ranges::sort(treeNodes, NodeSortCompare());
}

deflate::CodeTable::HuffmanCodeTable deflate::CodeTable::createCodeTable(const std::vector<std::uint8_t> &codeLengths, const std::uint16_t codeTableSize)
{
    if (codeLengths.empty())
    {
        return {};
    }

    for (const auto length: codeLengths)
    {
        if (length > MAX_BITS)
        {
            throw std::runtime_error(std::format("Unsupported Huffman code length {}. Maximum supported length is {}", length, MAX_BITS));
        }
    }

    if (codeLengths.size() == 1)
    {
        HuffmanCodeTable codeTable;
        CanonicalHuffmanCode code;
        code.code = 1;
        code.length = 1;
        codeTable[0] = code;
        return codeTable;
    }

    HuffmanCodeTable codeTable(codeTableSize);
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

deflate::CodeTable::ReverseHuffmanCodeTable deflate::CodeTable::createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths, const std::uint16_t codeTableSize)
{
    auto codeTable = createCodeTable(codeLengths, codeTableSize);
    ReverseHuffmanCodeTable reverseCodeTable(codeLengths.size());

    for (auto &[symbol, code]: codeTable)
    {
        const auto mask = (1 << static_cast<std::uint16_t>(code.length)) - 1;
        code.code = code.code & mask;
        reverseCodeTable[code] = symbol;
    }
    return reverseCodeTable;
}
