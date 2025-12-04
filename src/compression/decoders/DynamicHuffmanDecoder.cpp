//
// Created by cx9ps3 on 26.08.2024.
//

#include "DynamicHuffmanDecoder.hpp"
#include <array>
#include <ranges>

void deflate::DynamicHuffmanDecoder::decodeHeader()
{
    //read count of literals in table
    HLIT = static_cast<std::uint8_t>(bitBuffer->readBits(5));
    assert((HLIT + 257) > 0, "HLIT less or equal zero");

    //read count of distances in table
    HDIST = static_cast<std::uint8_t>(bitBuffer->readBits(5));
    assert((HDIST + 1) > 0, "HDIST less or equal zero");

    //read count of ccls
    HCLEN = static_cast<std::uint8_t>(bitBuffer->readBits(4));
    assert((HCLEN + 4) > 0, "HCLEN less or equal zero");

    decodeCodeLengths();
}

std::vector<std::uint8_t> deflate::DynamicHuffmanDecoder::decodeCCL()
{
    std::vector<std::uint8_t> ccl(19, 0);
    constexpr std::array<std::uint8_t, 19> permuteOrder = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    for (std::uint16_t i = 0; i < (HCLEN + 4); ++i)
    {
        ccl[permuteOrder[i]] = static_cast<std::uint8_t>(bitBuffer->readBits(3));
    }

    return ccl;
}

std::uint32_t deflate::DynamicHuffmanDecoder::reverseBits(const std::uint32_t bits, const std::uint8_t bitsCount) const
{
    std::uint16_t reversedCode = 0;
    for (std::uint8_t j = 0; j < bitsCount; ++j)
    {
        reversedCode |= ((bits >> j) & 1U) << (bitsCount - 1U - j);
    }
    return reversedCode;
}

std::optional<std::uint16_t> deflate::DynamicHuffmanDecoder::tryDecodeLength(const std::uint16_t lengthFixedCode)
{
    if (lengthFixedCode < 257 || lengthFixedCode > 285)
    {
        return std::nullopt;
    }

    const auto index = lengthFixedCode - 257;
    const auto base = LENGTH_BASE[index];
    const auto extra = LENGTH_EXTRA[index];
    if (extra == 0)
    {
        return base;
    }

    const auto extraBits = bitBuffer->readBits(extra);
    return base + extraBits;
}


std::optional<std::uint16_t> deflate::DynamicHuffmanDecoder::tryDecodeDistance(const std::uint32_t code, const std::uint8_t codeBitPosition)
{
    const auto it = std::ranges::find_if(distancesCodeTable, [code, codeBitPosition](const auto &pair)
                                         { return (pair.first.code == code) && (pair.first.length == codeBitPosition); });

    std::uint16_t fixedCode = 0;
    const auto findByIndex = [&fixedCode](const auto &element)
    {
        return ((element.index) == fixedCode) && (element.distance != 0);
    };

    if (it != distancesCodeTable.end())
    {
        fixedCode = it->second;
        if (const auto distanceCodesIterator = std::ranges::find_if(FIXED_DISTANCES_CODES, findByIndex); distanceCodesIterator != FIXED_DISTANCES_CODES.end())
        {
            std::uint16_t extraBits = 0;
            if (distanceCodesIterator->extraBitsCount > 0)
            {
                extraBits = static_cast<std::uint16_t>(bitBuffer->readBits(distanceCodesIterator->extraBitsCount));
            }

            return distanceCodesIterator->distance + extraBits;
        }
    }

    return std::nullopt;
}

std::optional<std::uint16_t> deflate::DynamicHuffmanDecoder::tryDecodeDistance(std::uint16_t symbol)
{
    if (symbol >= 30)
    {
        return std::nullopt;
    }
    const auto base = DISTANCE_BASE[symbol];
    const auto extra = DISTANCE_EXTRA[symbol];

    if (extra == 0)
    {
        return base;
    }

    const auto extraBits = bitBuffer->readBits(extra);
    return base + extraBits;
}

void deflate::DynamicHuffmanDecoder::decodeCodeLengths()
{
    const auto ccl = decodeCCL();
    auto reverseCodeTable = deflate::CodeTable::createReverseCodeTable(ccl, 19);

    const auto codeLengthsCount = HLIT + 257 + HDIST + 1;
    literalsCodeLengths.reserve(HLIT + 257);
    distanceCodeLengths.reserve(HDIST + 1);

    std::uint16_t i{0};
    std::uint16_t code{0};
    std::uint8_t codeBitPosition{0};
    std::uint16_t previousLength{0};
    std::uint16_t previousCode{0};

    auto repeatNumbers = [&i, this](const std::uint32_t repeat, const std::uint8_t number)
    {
        for (std::uint32_t j = 0; j < repeat; ++j)
        {
            if (i < (HLIT + 257))
            {
                literalsCodeLengths.push_back(number);
            }
            else
            {
                distanceCodeLengths.push_back(number);
            }
            ++i;
        }
    };

    while (i < codeLengthsCount)
    {
        code |= static_cast<std::uint32_t>(std::to_integer<uint16_t>(bitBuffer->readBit()) << codeBitPosition);
        ++codeBitPosition;


        const auto reversedBits = reverseBits(code, codeBitPosition);
        const auto it = std::ranges::find_if(reverseCodeTable, [reversedBits, codeBitPosition](const auto &pair)
                                             { return pair.first.code == reversedBits && pair.first.length == codeBitPosition; });

        if (it != reverseCodeTable.end())
        {
            const auto symbol = it->second;

            if (symbol == 18)
            {
                const auto repeat = bitBuffer->readBits(7);
                repeatNumbers(repeat + 11, 0);
            }
            else if (symbol == 17)
            {
                const auto repeat = bitBuffer->readBits(3);
                repeatNumbers(repeat + 3, 0);
            }
            else if (symbol == 16)
            {
                const auto repeat = bitBuffer->readBits(2);
                repeatNumbers(repeat + 3, previousLength);
            }
            else
            {
                repeatNumbers(1, symbol);
            }

            previousCode = symbol;
            if (symbol < 16)
                previousLength = symbol;

            code = 0;
            codeBitPosition = 0;
        }
    }
}

std::vector<deflate::LZ77::Match> deflate::DynamicHuffmanDecoder::decodeBody()
{
    std::vector<LZ77::Match> matches;

    std::uint32_t reversedCode = 0;
    std::uint8_t codeBitPosition = 0;

    std::uint16_t distance = 0;
    std::uint16_t length = 0;

    bool isEndOfBlock = false;

    const auto resetCode = [&]()
    {
        reversedCode = 0;
        codeBitPosition = 0;
    };

    // build normal reverse tables
    literalsCodeTable = CodeTable::createReverseCodeTable(
            literalsCodeLengths,
            FixedHuffmanEncoder::LITERALS_AND_LENGTHS_ALPHABET_SIZE);
    distancesCodeTable = CodeTable::createReverseCodeTable(
            distanceCodeLengths,
            FixedHuffmanEncoder::DISTANCES_ALPHABET_SIZE);


    // fallback search lambda
    const auto findCodeInCodeTable = [&](const auto &pair)
    {
        return (pair.first.code == reversedCode) && (pair.first.length == codeBitPosition);
    };

    while (!isEndOfBlock)
    {
        // ---- read next bit (LSB-first building of reversed code) ----
        const auto bit = std::to_integer<uint16_t>(bitBuffer->readBit());
        reversedCode = (reversedCode << 1) | bit;
        ++codeBitPosition;

        if (!isNextDistance)
        {
            CodeTable::CanonicalHuffmanCode huffmanCode{static_cast<std::uint16_t>(reversedCode), codeBitPosition};
            if (const auto it = literalsCodeTable.find(huffmanCode);
                it != literalsCodeTable.end())
            {
                const uint16_t symbol = it->second;

                if (symbol < 256)
                {
                    matches.emplace_back(std::byte{(uint8_t) symbol}, 0, 1);
                    resetCode();
                }
                else if (symbol == 256)
                {
                    isEndOfBlock = true;
                }
                else if (auto lenOpt = tryDecodeLength(symbol); lenOpt.has_value())
                {
                    length = lenOpt.value();
                    isNextDistance = true;
                    resetCode();
                }

                continue;
            }
        }

        // ---- FALLBACK: full search distance ----
        if (isNextDistance)
        {
            if (auto distOpt = tryDecodeDistance(reversedCode, codeBitPosition); distOpt.has_value())
            {
                distance = distOpt.value();
                isNextDistance = false;
                resetCode();
                continue;
            }
        }

        // ---- Output match when both decoded ----
        if (distance != 0 && length != 0)
        {
            matches.emplace_back(std::byte{0}, distance, length);
            distance = 0;
            length = 0;
        }
    }

    return matches;
}

std::vector<deflate::LZ77::Match> deflate::DynamicHuffmanDecoder::decodeData()
{
    decodeHeader();
    return decodeBody();
}

std::size_t deflate::DynamicHuffmanDecoder::getBlockSize() const noexcept
{
    return bitBuffer->getByteIndex();
}

deflate::DynamicHuffmanDecoder::DynamicHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitsBuffer) : bitBuffer(newBitsBuffer)
{
}