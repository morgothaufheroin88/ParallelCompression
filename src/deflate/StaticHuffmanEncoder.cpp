//
// Created by cx9ps3 on 18.08.2024.
//

#include "StaticHuffmanEncoder.hpp"
#include "BitBuffer.hpp"
#include <array>
#include <cstddef>

constexpr auto deflate::StaticHuffmanEncoder::initializeFixedCodesForLiterals()
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

constexpr auto deflate::StaticHuffmanEncoder::initializeFixedCodesForDistances()
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

constexpr auto deflate::StaticHuffmanEncoder::initializeFixedCodesForLengths()
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

std::vector<std::byte> deflate::StaticHuffmanEncoder::encodeData(const std::vector<LZ77::Match> &dataToEncode, const bool isLastBlock)
{
    constexpr auto FIXED_LITERALS_CODES{initializeFixedCodesForLiterals()};
    constexpr auto FIXED_DISTANCES_CODES{initializeFixedCodesForDistances()};
    constexpr auto FIXED_LENGTHS_CODES{initializeFixedCodesForLengths()};
    std::vector<std::byte> compressedData;
    compressedData.reserve(dataToEncode.size());
    BitBuffer bitBuffer;

    //write header
    if (isLastBlock)
    {
        bitBuffer.writeBits(1, 1);
    }
    else
    {
        bitBuffer.writeBits(1, 0);
    }

    bitBuffer.writeBits(0b01, 2);


    for (const auto &[literal, distance, length]: dataToEncode)
    {
        if (length > 1)
        {
            const auto &lengthCode = FIXED_LENGTHS_CODES[length];
            const auto &distanceCode = FIXED_DISTANCES_CODES[distance];

            //encode lz77 lengths
            bitBuffer.writeBits(lengthCode.code, lengthCode.codeLength);
            bitBuffer.writeBits(lengthCode.extraBits, lengthCode.extraBitsCount);

            //encode lz77 distance
            bitBuffer.writeBits(distanceCode.code, 5);
            bitBuffer.writeBits(distanceCode.extraBits, distanceCode.extraBitsCount);
        }
        else
        {
            const auto &literalCode = FIXED_LITERALS_CODES[static_cast<std::uint8_t>(literal)];

            //encode lz77 literal
            bitBuffer.writeBits(literalCode.code, literalCode.codeLength);
        }
    }

    //write end of block
    bitBuffer.writeBits(0, 7);

    return bitBuffer.getBytes();
}