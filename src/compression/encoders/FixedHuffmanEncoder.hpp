//
// Created by cx9ps3 on 18.08.2024.
//

#pragma once
#include "../lz/LZ77.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace deflate
{
    class FixedHuffmanEncoder
    {
    public:
        static constexpr std::uint16_t MAX_LENGTH = 258;
        static constexpr std::uint16_t MAX_DISTANCE = 32'768;
        static constexpr std::int16_t MAX_LITERAL = 256;
        static constexpr std::uint16_t DISTANCES_ALPHABET_SIZE = 30;
        static constexpr std::uint16_t LITERALS_AND_DISTANCES_ALPHABET_SIZE = 285;

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

        template<std::size_t N>
        [[nodiscard]] static consteval auto createCodes(const std::array<std::uint8_t, N> &codesLengths)
        {
            std::array<std::uint16_t, N> codes{};
            std::array<std::uint16_t, 16> codeLengthsCount = {0};
            std::array<std::uint16_t, 16> nextCode = {0};
            const auto maxCodeBit = *std::ranges::max_element(codesLengths);

            for (const auto &length: codesLengths)
            {
                auto &value = codeLengthsCount[length];
                ++value;
            }

            std::uint16_t code{0};
            nextCode[1] = 0;
            for (std::uint8_t bit = 2; bit <= maxCodeBit; ++bit)
            {
                code = static_cast<std::uint16_t>((code + codeLengthsCount[bit - 1]) << 1);
                nextCode[bit] = code;
            }

            std::uint16_t symbol{0};
            for (const auto length: codesLengths)
            {
                if (length != 0)
                {
                    std::uint16_t reversedCode{0};
                    for (std::uint8_t j = 0; j < length; ++j)
                    {
                        reversedCode |= ((nextCode[length] >> j) & 1U) << (length - 1U - j);
                    }
                    codes[symbol] = reversedCode;
                    auto &value = nextCode[length];
                    ++value;
                }
                ++symbol;
            }

            return codes;
        }

        [[nodiscard]] static consteval auto initializeFixedCodesForLiteralsAndLengths()
        {
            std::array<LiteralCode, MAX_LITERAL + 1> literalsCodes{};
            std::array<LengthCode, MAX_LENGTH + 1> lengthsCodes{};
            std::array<std::uint8_t, 288> literalCodeLengths{};

            std::fill_n(literalCodeLengths.begin(), 144, 8);
            std::fill_n(literalCodeLengths.begin() + 144, 112, 9);
            std::fill_n(literalCodeLengths.begin() + 256, 24, 7);
            std::fill_n(literalCodeLengths.begin() + 280, 8, 8);

            //create codes for literals
            const auto codes = createCodes<288>(literalCodeLengths);
            for (std::uint16_t i = 0; i < (MAX_LITERAL + 1); ++i)
            {
                literalsCodes[i].code = codes[i];
                literalsCodes[i].codeLength = literalCodeLengths[i];
                literalsCodes[i].literal = static_cast<std::byte>(i);
            }

            //create codes for lengths
            const std::array<std::uint16_t, 256> lengthCodes = {
                    257, 258, 259, 260, 261, 262, 263, 264, 265, 265, 266, 266, 267, 267, 268, 268,
                    269, 269, 269, 269, 270, 270, 270, 270, 271, 271, 271, 271, 272, 272, 272, 272,
                    273, 273, 273, 273, 273, 273, 273, 273, 274, 274, 274, 274, 274, 274, 274, 274,
                    275, 275, 275, 275, 275, 275, 275, 275, 276, 276, 276, 276, 276, 276, 276, 276,
                    277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277, 277,
                    278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278, 278,
                    279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279, 279,
                    280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280, 280,
                    281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281,
                    281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281, 281,
                    282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282,
                    282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282, 282,
                    283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283,
                    283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283, 283,
                    284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284,
                    284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 284, 285};

            const std::array<std::uint16_t, 29> lengthBases =
                    {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195,
                     227, 258};

            const std::array<std::uint8_t, 29> lengthExtraBitsCount = {
                    0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2,
                    3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

            for (std::uint16_t i = 3; i <= MAX_LENGTH; ++i)
            {
                const auto lengthCode = lengthCodes[i - 3];
                lengthsCodes[i].index = static_cast<std::uint8_t>(lengthCode - 257);
                lengthsCodes[i].length = i;
                lengthsCodes[i].code = codes[lengthCode];
                lengthsCodes[i].codeLength = literalCodeLengths[lengthCode];
                lengthsCodes[i].extraBits = i - lengthBases[lengthCode - 257];
                lengthsCodes[i].extraBitsCount = lengthExtraBitsCount[lengthCode - 257];
            }


            return std::make_pair(literalsCodes, lengthsCodes);
        }

        [[nodiscard]] static consteval std::size_t findDistanceCode(const std::array<std::uint16_t, 30> &bases, const std::uint16_t distance)
        {
            const auto basesCount = bases.size();
            for (std::size_t i = 0; i < basesCount; ++i)
            {
                if (distance < bases[i])
                {
                    return i - 1;
                }
            }
            return bases.size() - 1;
        }

    public:
        [[nodiscard]] static consteval auto initializeFixedCodesForLiterals()
        {
            return initializeFixedCodesForLiteralsAndLengths().first;
        }


        [[nodiscard]] static consteval auto initializeFixedCodesForDistances()
        {
            std::array<DistanceCode, MAX_DISTANCE + 1> result{};
            std::array<std::uint8_t, 32> codeLengths{};
            std::fill_n(codeLengths.begin(), 32, 5);

            const auto codes = createCodes<32>(codeLengths);

            const std::array<std::uint16_t, 30> distanceBases =
                    {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073,
                     4097, 6145, 8193, 12'289, 16'385, 24'577};

            const std::array<std::uint16_t, 30> distanceExtraBitsCount = {
                    0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6,
                    7, 7, 8, 8, 9, 9, 10, 10, 11, 11,
                    12, 12, 13, 13};

            for (std::uint16_t i = 1; i <= MAX_DISTANCE; ++i)
            {
                const auto distanceCode = findDistanceCode(distanceBases, i);
                result[i].index = static_cast<std::uint8_t>(distanceCode);
                result[i].distance = i;
                result[i].code = static_cast<std::uint8_t>(codes[distanceCode]);
                result[i].extraBits = i - distanceBases[distanceCode];
                result[i].extraBitsCount = static_cast<std::uint8_t>(distanceExtraBitsCount[distanceCode]);
            }

            return result;
        }

        [[nodiscard]] static consteval auto initializeFixedCodesForLengths()
        {
            return initializeFixedCodesForLiteralsAndLengths().second;
        }

        [[nodiscard]] static std::vector<std::byte> encodeData(const std::vector<LZ77::Match> &dataToEncode, bool isLastBlock);
    };
}// namespace deflate