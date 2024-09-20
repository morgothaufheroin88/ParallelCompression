//
// Created by cx9ps3 on 18.08.2024.
//

#pragma once
#include "../lz/LZ77.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace deflate
{
    class FixedHuffmanEncoder
    {
    private:
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

    public:
        static constexpr std::uint16_t MAX_LENGTH = 258;
        static constexpr std::uint16_t MAX_DISTANCE = 32'768;
        static constexpr std::int16_t MAX_LITERAL = 255;
        static constexpr std::uint16_t DISTANCES_ALPHABET_SIZE = 30;
        static constexpr std::uint16_t LITERALS_AND_DISTANCES_ALPHABET_SIZE = 285;

        [[nodiscard]] static constexpr auto initializeFixedCodesForLiterals()
        {
            std::array<LiteralCode, MAX_LITERAL + 1> result{};
            std::uint16_t start = 0b00'11'00'00;
            std::uint8_t countOfBits = 8;
            for (std::uint8_t i = 0; i < MAX_LITERAL; ++i)
            {
                if (i == 144)
                {
                    countOfBits = 9;
                    start = 0b110'010'000;
                }

                std::uint16_t reversedCode = 0;
                for (std::uint8_t j = 0; j < countOfBits; ++j)
                {
                    reversedCode |= ((start >> j) & 1U) << (countOfBits - 1U - j);
                }

                result[i].code = reversedCode;
                result[i].codeLength = countOfBits;
                result[i].literal = static_cast<std::byte>(i);

                ++start;
            }
            return result;
        }


        [[nodiscard]] static constexpr auto initializeFixedCodesForDistances()
        {
            std::array<DistanceCode, MAX_DISTANCE + 1> result{};
            std::uint8_t bits{0};
            std::uint16_t mask = 0;
            std::uint16_t lastDistance = 1;
            for (std::uint8_t i = 0; i < DISTANCES_ALPHABET_SIZE; ++i)
            {
                if ((i > 2) && ((i % 2) == 0))
                {
                    ++bits;
                    for (std::uint8_t j = 0; j < bits; ++j)
                    {
                        mask = static_cast<uint16_t>((1 << (j + 1u)) - 1);
                    }
                }

                std::byte reversedCode{0};
                for (std::uint8_t j = 0; j < 5; ++j)
                {
                    reversedCode |= ((std::byte{i} >> j) & std::byte{1}) << (4 - j);
                }

                for (std::uint16_t j = 0; j <= mask; ++j)
                {
                    DistanceCode code{};

                    code.code = std::to_integer<std::uint8_t>(reversedCode);
                    code.distance = lastDistance;
                    code.extraBitsCount = bits;
                    code.extraBits = j;
                    code.index = i;

                    result[lastDistance] = code;
                    ++lastDistance;
                }
            }
            return result;
        }

        [[nodiscard]] static constexpr auto initializeFixedCodesForLengths()
        {
            std::array<LengthCode, MAX_LENGTH + 1> result{};
            std::uint16_t start = 0;
            std::uint8_t countOfBits = 7;
            std::uint8_t extraBitsCount{0};
            std::uint16_t length{1};
            std::uint8_t mask{0};
            for (std::uint16_t i = MAX_LITERAL + 1; i < LITERALS_AND_DISTANCES_ALPHABET_SIZE; ++i)
            {
                if (i == 280)
                {
                    countOfBits = 8;
                    start = 0b11'00'00'00;
                }
                if (i < 265)
                {
                    ++length;
                }
                else
                {
                    if ((i % 4) == 1)
                    {
                        ++extraBitsCount;
                    }
                }

                for (std::uint8_t j = 0; j < extraBitsCount; ++j)
                {
                    mask = static_cast<std::uint8_t>((1U << (j + 1U)) - 1U);
                }

                std::uint16_t reversedCode = 0;
                for (std::uint8_t j = 0; j < countOfBits; ++j)
                {
                    reversedCode |= ((start >> j) & 1U) << (countOfBits - 1U - j);
                }

                for (std::uint16_t j = 0; j <= mask; ++j)
                {

                    result[length].index = static_cast<std::uint8_t>(i - 255);
                    result[length].length = length;
                    result[length].code = reversedCode;
                    result[length].codeLength = countOfBits;
                    result[length].extraBits = j;
                    result[length].extraBitsCount = extraBitsCount;
                    if (i > 265)
                    {
                        ++length;
                    }
                }

                ++start;
            }

            result[258].extraBitsCount = 0;
            result[258].extraBits = 0;

            return result;
        }

        [[nodiscard]] static std::vector<std::byte> encodeData(const std::vector<LZ77::Match> &dataToEncode, bool isLastBlock);
    };
}// namespace deflate