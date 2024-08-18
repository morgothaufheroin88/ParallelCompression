//
// Created by cx9ps3 on 18.08.2024.
//

#pragma once
#include "LZ77.hpp"
#include <cstdint>
#include <vector>

namespace deflate
{
    class StaticHuffmanEncoder
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
        [[nodiscard]] static constexpr auto initializeFixedCodesForLiterals();
        [[nodiscard]] static constexpr auto initializeFixedCodesForDistances();
        [[nodiscard]] static constexpr auto initializeFixedCodesForLengths();
        [[nodiscard]] static std::vector<std::byte> encodeData(const std::vector<LZ77::Match> &dataToEncode, bool isLastBlock);
    };
}// namespace deflate