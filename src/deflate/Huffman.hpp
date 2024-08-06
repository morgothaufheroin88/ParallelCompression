//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include "LZ77.hpp"
#include <array>
#include <unordered_map>

namespace deflate
{
    class Huffman
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

        struct NodeSort
        {
            bool operator()(const Node &left, const Node &right) const
            {
                if (left.codeLength == right.codeLength)
                {
                    return left.symbol < right.symbol;
                }

                return left.codeLength < right.codeLength;
            }
        };

        struct LiteralCode
        {
            std::uint16_t code;
            std::uint8_t codeLength;
        };

        struct LengthCode
        {
            std::uint16_t length;
            std::uint16_t code;
            std::uint8_t codeLength;
            std::uint8_t extraBitsCount;
            std::uint16_t extraBits;
        };
        struct DistanceCode
        {
            std::uint8_t code;
            std::uint16_t distance;
            std::uint8_t extraBitsCount;
            std::uint16_t extraBits;
        };

        static std::uint16_t reverseBits(std::uint16_t n, std::uint8_t bitCount)
        {
            std::uint16_t result = 0;
            for (std::uint8_t i = 0; i < bitCount; ++i)
            {
                result |= ((n >> i) & 1) << (bitCount - 1 - i);
            }
            return result;
        }


        using TreeNodes = std::vector<Node>;
        static constexpr std::uint16_t LITERALS_AND_LENGTHS_ALPHABET_SIZE = 286;
        static constexpr auto FIXED_LITERALS_CODES{
                []() constexpr
                {
                    std::array<LiteralCode, 256> result{};
                    std::uint16_t start = 0b00110000;
                    std::uint8_t countOfBits = 8;
                    for (auto i = 0; i < 256; ++i)
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
                        start++;
                    }
                    return result;
                }()};


        static constexpr std::uint16_t DISTANCES_ALPHABET_SIZE = 30;
        static constexpr auto FIXED_DISTANCES_CODES{
                []() constexpr
                {
                    std::array<DistanceCode, 32769> result{};
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

                            result[lastDistance] = code;
                            lastDistance++;
                        }
                    }
                    return result;
                }()};

        static constexpr auto FIXED_LENGTHS_CODES{
                []() constexpr
                {
                    std::array<LengthCode, 259> result{};
                    std::uint16_t start = 0b0000000;
                    std::uint8_t countOfBits = 7;
                    std::uint8_t extraBitsCount{0};
                    std::uint16_t length{1};
                    std::uint8_t mask{0};
                    for (auto i = 256; i < 285; i++)
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

        static constexpr std::uint8_t MAX_BITS = 15;
        static std::pair<std::vector<std::uint16_t>, std::vector<uint16_t>> lz77MatchesToVectors(const std::vector<LZ77::Match> &lz77Matches);
        static std::vector<std::uint32_t> countFrequencies(const std::vector<std::uint16_t> &data, std::uint16_t alphabetSize);
        static TreeNodes buildTree(const std::vector<std::uint32_t> &frequencies);
        static void calculateCodesLengths(TreeNodes &treeNodes, std::uint32_t rootIndex);
        static std::unordered_map<std::uint16_t, std::uint16_t> createCodeTable(const std::vector<std::uint8_t> &codeLengths);
        static std::unordered_map<std::uint16_t, std::uint16_t> createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths);
        static void addBitsToBuffer(std::vector<std::byte> &buffer, std::uint16_t value, std::uint8_t bitCount, std::uint8_t &bitPosition, std::byte &currentByte);

    public:
        static std::vector<std::byte> encodeWithDynamicCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock);
        static std::vector<std::byte> encodeWithFixedCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock);
    };
}// namespace deflate
