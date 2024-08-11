//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include "LZ77.hpp"
#include <array>
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

        struct LiteralCode
        {
            std::uint16_t code;
            std::uint8_t codeLength;
            std::byte literal;
        };

        struct LengthCode
        {
            std::uint16_t length;
            std::uint16_t code;
            std::uint8_t codeLength;
            std::uint8_t extraBitsCount;
            std::uint16_t extraBits;
            std::uint8_t index;
        };
        struct DistanceCode
        {
            std::uint8_t code;
            std::uint16_t distance;
            std::uint8_t extraBitsCount;
            std::uint16_t extraBits;
            std::uint8_t index;
        };

        using TreeNodes = std::vector<Node>;
        using Frequencies = std::tuple<std::vector<uint32_t>,std::vector<uint32_t> ,std::vector<uint32_t>>;
        using MinimalHeap = std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, NodeCompare>;

        static constexpr std::uint16_t MAX_LENGTH  = 258;
        static constexpr std::uint16_t MAX_DISTANCE = 32768;
        static constexpr std::uint16_t MAX_LITERAL = 255;
        static constexpr std::uint16_t DISTANCES_ALPHABET_SIZE = 30;
        static constexpr std::uint16_t LITERALS_AND_DISTANCES_ALPHABET_SIZE = 285;
        static constexpr std::uint8_t MAX_BITS = 15;

        static constexpr auto FIXED_LITERALS_CODES{
                []() constexpr
                {
                    std::array<LiteralCode, MAX_LITERAL + 1> result{};
                    std::uint16_t start = 0b00110000;
                    std::uint8_t countOfBits = 8;
                    for (auto i = 0; i < MAX_LITERAL; ++i)
                    {
                        if (i == 144)
                        {
                            countOfBits = 9;
                            start = 0b110010000;
                        }

                        std::uint16_t reversedCode = 0;
                        for (std::uint8_t j = 0; j < countOfBits; ++j)
                        {
                            reversedCode |= ((start >> j) & 1) << (countOfBits - 1 - j);
                        }

                        result[i].code = reversedCode;
                        result[i].codeLength = countOfBits;
                        result[i].literal = static_cast<std::byte>(i);

                        start++;
                    }
                    return result;
                }()};


        static constexpr auto FIXED_DISTANCES_CODES{
                []() constexpr
                {
                    std::array<DistanceCode, MAX_DISTANCE + 1> result{};
                    std::uint8_t bits{0};
                    std::uint16_t mask = 0;
                    std::uint16_t lastDistance = 1;
                    for (std::uint8_t i = 0; i < DISTANCES_ALPHABET_SIZE; i++)
                    {
                        if ((i > 2) && (i % 2) == 0)
                        {
                            bits++;
                            for (std::uint8_t j = 0; j < bits; j++)
                            {
                                mask = static_cast<uint16_t>((1 << (j + 1)) - 1);
                            }
                        }

                        std::byte reversedCode{0};
                        for (std::uint8_t j = 0; j < 5; ++j)
                        {
                            reversedCode |= ((std::byte(i) >> j) & std::byte(1)) << (4 - j);
                        }

                        for (std::uint16_t j = 0; j <= mask; j++)
                        {
                            DistanceCode code{};

                            code.code = std::to_integer<std::uint8_t>(reversedCode);
                            code.distance = lastDistance;
                            code.extraBitsCount = bits;
                            code.extraBits = j;
                            code.index = i;

                            result[lastDistance] = code;
                            lastDistance++;
                        }
                    }
                    return result;
                }()};

        static constexpr auto FIXED_LENGTHS_CODES{
                []() constexpr
                {
                    std::array<LengthCode, MAX_LENGTH + 1> result{};
                    std::uint16_t start = 0b0000000;
                    std::uint8_t countOfBits = 7;
                    std::uint8_t extraBitsCount{0};
                    std::uint16_t length{1};
                    std::uint8_t mask{0};
                    for (std::uint16_t i = MAX_LITERAL + 1; i < LITERALS_AND_DISTANCES_ALPHABET_SIZE; i++)
                    {
                        if (i == 280)
                        {
                            countOfBits = 8;
                            start = 0b11000000;
                        }
                        if (i < 265)
                        {
                            length++;
                        }
                        else
                        {
                            if ((i % 4) == 1)
                            {
                                extraBitsCount++;
                            }
                        }

                        for (std::uint8_t j = 0; j < extraBitsCount; j++)
                        {
                            mask = static_cast<std::uint8_t>((1 << (j + 1)) - 1);
                        }

                        std::uint16_t reversedCode = 0;
                        for (std::uint8_t j = 0; j < countOfBits; ++j)
                        {
                            reversedCode |= ((start >> j) & 1) << (countOfBits - 1 - j);
                        }

                        for (std::uint16_t j = 0; j <= mask; j++)
                        {

                            result[length].index = static_cast<std::uint8_t>(i - 256);
                            result[length].length = length;
                            result[length].code = reversedCode;
                            result[length].codeLength = countOfBits;
                            result[length].extraBits = j;
                            result[length].extraBitsCount = extraBitsCount;
                            if (i > 265)
                            {
                                length++;
                            }
                        }
                        start++;
                    }

                    result[258].extraBitsCount = 0;
                    result[258].extraBits = 0;

                    return result;
                }()};

        static deflate::Huffman::Frequencies countFrequencies(const std::vector<LZ77::Match> &lz77Matches);
        static TreeNodes buildLiteralsAndLengthsTree(const std::vector<std::uint32_t> &literalsFrequencies, const std::vector<std::uint32_t> &lengthsFrequencies);
        static void buildTree(MinimalHeap& minimalHeap,TreeNodes &treeNodes);
        static TreeNodes buildCanonicalTree(const TreeNodes &standardTreeNodes);
        static void calculateCodesLengths(TreeNodes &treeNodes, std::uint32_t rootIndex);
        static void createCodes(TreeNodes &treeNodes, std::uint32_t rootIndex);
        static std::unordered_map<std::uint16_t, std::uint16_t> createCodeTable(const std::vector<std::uint8_t> &codeLengths);
        static std::unordered_map<std::uint16_t, std::uint16_t> createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths);
        static void addBitsToBuffer(std::vector<std::byte> &buffer, std::uint16_t value, std::uint8_t bitCount, std::uint8_t &bitPosition, std::byte &currentByte);

    public:
        static std::vector<std::byte> encodeWithDynamicCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock);
        static std::vector<std::byte> encodeWithFixedCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock);
        static std::vector<LZ77::Match> decodeWithFixedCodes(const std::vector<std::byte> &bitsCount);
    };
}// namespace deflate
