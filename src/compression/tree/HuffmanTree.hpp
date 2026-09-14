//
// Created by cx9ps3 on 19.08.2024.
//

#pragma once
#include <cstdint>
#include <queue>
#include <unordered_map>
#include <vector>

namespace deflate
{
    class HuffmanTree
    {
    private:
        struct Node
        {
            std::uint32_t parentId{0};
            std::int32_t leftChildId{-1};
            std::int32_t rightChildId{-1};
            std::uint16_t code{0};
            std::int16_t symbol{-1};
            std::uint8_t codeLength{0};
            std::uint32_t frequency{0};
        };
        using TreeNodes = std::vector<Node>;

        /// Which of two nodes the heap should give up first: the rarer one,
        /// and between two equally rare the earlier, so that the same input
        /// always makes the same tree.
        struct NodeCompare
        {
            const TreeNodes *nodes;
            bool operator()(const std::uint32_t left, const std::uint32_t right) const
            {
                const auto &a = (*nodes)[left];
                const auto &b = (*nodes)[right];
                return a.frequency != b.frequency ? a.frequency > b.frequency : left > right;
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

        using MinimalHeap = std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, NodeCompare>;

        TreeNodes treeNodes;

        [[nodiscard]] std::vector<std::uint32_t> countFrequencies(const std::vector<std::int16_t> &symbols, std::size_t alphabetSize) const;
        void createNodes(const std::vector<std::uint32_t> &frequencies, MinimalHeap &minimalHeap);
        void buildTree(MinimalHeap &minimalHeap);
        void calculateCodesLengths(std::int32_t rootIndex);
        void limitCodeLengths(std::uint8_t maxLength);

    public:
        [[nodiscard]] std::vector<std::uint8_t> getLengthsFromNodes(std::uint16_t size) const;

        /**
         * @param symbols What was coded, one entry per occurrence.
         * @param alphabetSize How many symbols there could be.
         * @param maxLength The longest code the format allows here: fifteen
         *        bits for literals and distances, seven for the code the
         *        lengths themselves are written in (RFC 1951 3.2.7).  A tree
         *        that comes out deeper is flattened to fit, at the cost of a
         *        few bits on the rarest symbols.
         */
        explicit HuffmanTree(const std::vector<std::int16_t> &symbols, std::size_t alphabetSize, std::uint8_t maxLength = 15);
    };

    class CodeTable
    {
    public:
        struct CanonicalHuffmanCode
        {
            std::uint16_t code{0};
            std::uint8_t length{0};
            bool operator==(const CanonicalHuffmanCode &other) const = default;
        };

    private:
        class CanonicalHuffmanCodesHash
        {
        public:
            std::size_t operator()(const CanonicalHuffmanCode &code) const
            {
                return std::hash<std::uint32_t>()(code.code + code.length);
            }
        };
        static constexpr std::uint8_t MAX_BITS = 15;

    public:
        using HuffmanCodeTable = std::unordered_map<std::uint16_t, CanonicalHuffmanCode>;
        using ReverseHuffmanCodeTable = std::unordered_map<CanonicalHuffmanCode, std::uint16_t, CanonicalHuffmanCodesHash>;
        static HuffmanCodeTable createCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize);
        static ReverseHuffmanCodeTable createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize);
    };
}// namespace deflate
