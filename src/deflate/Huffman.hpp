//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include "LZ77.hpp"
#include <array>
#include <optional>
#include <queue>
#include <unordered_map>

namespace deflate
{
    class Huffman
    {
    private:
        enum class NodeType
        {
            NONE = 0,
            LITERAL,
            LENGTH,
            DISTANCE,
        };

        struct Node
        {
            std::uint32_t parentId{0};
            std::int32_t leftChildId{-1};
            std::int32_t rightChildId{-1};
            std::uint16_t code{0};
            std::int16_t symbol{-1};
            std::uint8_t codeLength{0};
            std::uint32_t frequency{0};
            NodeType nodeType{NodeType::NONE};
        };


        struct NodeCompare
        {
            bool operator()(const std::uint32_t left, const std::uint32_t right) const
            {
                return left > right;
            }
        };

        struct NodeSortCompare
        {
            bool operator()(const Node &a, const Node &b) const
            {
                if (a.codeLength == b.codeLength)
                {
                    return a.symbol < b.symbol;
                }
                return a.codeLength < b.codeLength;
            }
        };

        struct CanonicalHuffmanCode
        {
            std::uint16_t code{0};
            std::uint8_t length{0};
        };

        using TreeNodes = std::vector<Node>;
        using Frequencies = std::tuple<std::vector<uint32_t>, std::vector<uint32_t>, std::vector<uint32_t>>;
        using MinimalHeap = std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, NodeCompare>;
        using DynamicCodeTable = std::unordered_map<std::uint16_t, CanonicalHuffmanCode>;
        static constexpr std::uint8_t MAX_BITS = 15;

        static std::vector<std::uint16_t> createShortSequence(const std::vector<std::uint8_t> &codeLengths);
        static void createRLECodes(std::vector<std::uint16_t> &rleCodes, std::uint8_t length, std::int16_t count);
        static DynamicCodeTable createCCL(const std::vector<std::int16_t> &rleCodes);
        static Frequencies countFrequencies(const std::vector<LZ77::Match> &lz77Matches);
        static TreeNodes buildLiteralsAndLengthsTree(const std::vector<std::uint32_t> &literalsFrequencies, const std::vector<std::uint32_t> &lengthsFrequencies);
        static TreeNodes buildDistancesTree(const std::vector<std::uint32_t> &distancesFrequencies);
        static void buildTree(MinimalHeap &minimalHeap, TreeNodes &treeNodes);
        static void calculateCodesLengths(TreeNodes &treeNodes, std::uint32_t rootIndex);
        static std::vector<std::uint8_t> getLengthsFromNodes(const TreeNodes &treeNodes, std::uint16_t size);
        static DynamicCodeTable createCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize);
        static DynamicCodeTable createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize);
        static DynamicCodeTable createCodeTableForLiterals(const std::vector<uint32_t> &literalsFrequencies, const std::vector<uint32_t> &lengthsFrequencies);
        static DynamicCodeTable createCodeTableForDistances(const std::vector<uint32_t> &distancesFrequencies);
        static void addBitsToBuffer(std::vector<std::byte> &buffer, std::uint16_t value, std::uint8_t bitCount, std::uint8_t &bitPosition, std::byte &currentByte);

    public:
        static std::vector<std::byte> encodeWithDynamicCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock);
        static std::vector<LZ77::Match> decodeWithFixedCodes(const std::vector<std::byte> &bitsCount);
    };
}// namespace deflate
