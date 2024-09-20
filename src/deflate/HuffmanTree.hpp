//
// Created by cx9ps3 on 19.08.2024.
//

#pragma once
#include <cstdint>
#include <queue>
#include <unordered_map>

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

        using TreeNodes = std::vector<Node>;
        using MinimalHeap = std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, NodeCompare>;

        MinimalHeap minimalHeap;
        TreeNodes treeNodes;

        [[nodiscard]] std::vector<std::uint32_t> countFrequencies(const std::vector<std::int16_t> &symbols, std::size_t alphabetSize) const;
        void createNodes(const std::vector<std::uint32_t> &frequencies);
        void buildTree();
        void calculateCodesLengths(std::int32_t rootIndex);

    public:
        [[nodiscard]] std::vector<std::uint8_t> getLengthsFromNodes(std::uint16_t size) const;
        explicit HuffmanTree(const std::vector<std::int16_t> &symbols, std::size_t alphabetSize);
    };

    class CodeTable
    {
    private:
        struct CanonicalHuffmanCode
        {
            std::uint16_t code{0};
            std::uint8_t length{0};
            bool operator==(const CanonicalHuffmanCode &other) const = default;
        };

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
        using DynamicCodeTable = std::unordered_map<std::uint16_t, CanonicalHuffmanCode>;
        using ReverseDynamicCodeTable = std::unordered_map<CanonicalHuffmanCode, std::uint16_t, CanonicalHuffmanCodesHash>;
        static DynamicCodeTable createCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize);
        static ReverseDynamicCodeTable createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize);
    };
}// namespace deflate
