//
// Created by cx9ps3 on 26.08.2024.
//

#include "DynamicHuffmanDecoder.hpp"
#include <array>
#include <ranges>

void deflate::DynamicHuffmanDecoder::decodeHeader()
{
    //read block type
    [[maybe_unused]] const auto isLastBlock = bitBuffer.readBit();
    const auto blockType = bitBuffer.readBits(2);
    assert(blockType == 0b10, "Wrong block type!");

    //read count of literals in table
    HLIT = static_cast<std::uint8_t>(bitBuffer.readBits(5));
    assert((HLIT + 257) > 0, "HLIT less or equal zero");

    //read count of distances in table
    HDIST = static_cast<std::uint8_t>(bitBuffer.readBits(5));
    assert((HDIST + 1) > 0, "HDIST less or equal zero");

    //read count of ccls
    HCLEN = static_cast<std::uint8_t>(bitBuffer.readBits(4));
    assert((HCLEN + 4) > 0, "HCLEN less or equal zero");

    decodeCodeLengths();
}

std::vector<std::uint8_t> deflate::DynamicHuffmanDecoder::decodeCCL()
{
    std::vector<std::uint8_t> ccl(19, 0);
    constexpr std::array<std::uint8_t, 19> permuteOrder = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    for (std::uint16_t i = 0; i < (HCLEN + 4); ++i)
    {
        ccl[permuteOrder[i]] = static_cast<std::uint8_t>(bitBuffer.readBits(3));
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
    const auto findByIndex = [&lengthFixedCode](const auto &element)
    {
        return ((element.index + 257) == lengthFixedCode) && (element.code != 0) && (element.codeLength != 0);
    };

    if (const auto lengthCodesIterator = std::ranges::find_if(FIXED_LENGTHS_CODES, findByIndex); lengthCodesIterator != FIXED_LENGTHS_CODES.end())
    {
        std::uint16_t extraBits = 0;
        if (lengthCodesIterator->extraBitsCount > 0)
        {
            extraBits = static_cast<std::uint16_t>(bitBuffer.readBits(lengthCodesIterator->extraBitsCount));
        }

        return lengthCodesIterator->length + extraBits;
    }

    return std::nullopt;
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
                extraBits = static_cast<std::uint16_t>(bitBuffer.readBits(distanceCodesIterator->extraBitsCount));
            }

            return distanceCodesIterator->distance + extraBits;
        }
    }

    return std::nullopt;
}

void deflate::DynamicHuffmanDecoder::decodeCodeLengths()
{
    const auto ccl = decodeCCL();
    const auto reverseCodeTable = deflate::CodeTable::createReverseCodeTable(ccl, 19);
    const auto codeLengthsCount = HLIT + 257 + HDIST + 1;
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
        //read one bit from byte
        const auto bit = bitBuffer.readBit();
        code |= static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(bit) << codeBitPosition);
        ++codeBitPosition;

        const auto reversedBits = reverseBits(code, codeBitPosition);

        const auto it = std::ranges::find_if(reverseCodeTable, [reversedBits, codeBitPosition](const auto &pair)
                                             { return (pair.first.code == reversedBits) && (pair.first.length == codeBitPosition); });

        if (it != reverseCodeTable.end())
        {
            if (it->second == 18)
            {
                const auto repeat = bitBuffer.readBits(7);
                repeatNumbers(repeat + 11, 0);
            }
            else if (it->second == 17)
            {
                const auto repeat = bitBuffer.readBits(3);
                repeatNumbers(repeat + 3, 0);
            }
            else if ((previousCode < 16) && (previousCode > 0) && (it->second == 16))
            {
                previousLength = previousCode;
                const auto repeat = bitBuffer.readBits(2);
                repeatNumbers(repeat + 3, static_cast<std::uint8_t>(previousLength));
            }
            else if ((previousCode == 16) && (it->second == 16))
            {
                const auto repeat = bitBuffer.readBits(2);
                repeatNumbers(repeat + 3, static_cast<std::uint8_t>(previousLength));
            }
            else
            {
                repeatNumbers(1, static_cast<std::uint8_t>(it->second));
            }

            previousCode = it->second;
            code = 0;
            codeBitPosition = 0;
        }
    }
}

std::vector<deflate::LZ77::Match> deflate::DynamicHuffmanDecoder::decodeBody()
{
    std::vector<LZ77::Match> matches;
    std::uint16_t code{0};
    std::uint32_t reversedCode{0};
    std::uint8_t codeBitPosition{0};
    std::uint16_t distance{0};
    std::uint16_t length{0};
    bool isEndOfBlock{false};

    const auto findCodeInCodeTable = [&reversedCode, &codeBitPosition](const auto &pair)
    { return (pair.first.code == reversedCode) && (pair.first.length == codeBitPosition); };

    literalsCodeTable = CodeTable::createReverseCodeTable(literalsCodeLengths, FixedHuffmanEncoder::LITERALS_AND_LENGTHS_ALPHABET_SIZE);
    distancesCodeTable = CodeTable::createReverseCodeTable(distanceCodeLengths, FixedHuffmanEncoder::DISTANCES_ALPHABET_SIZE);


    const auto resetCode = [&code, &reversedCode, &codeBitPosition]()
    {
        reversedCode = 0;
        code = 0;
        codeBitPosition = 0;
    };

    while ((!isEndOfBlock)  || (bitBuffer.next()))
    {
        //read one bit from byte
        const auto bit = bitBuffer.readBit();
        code |= static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(bit) << codeBitPosition);
        ++codeBitPosition;

        reversedCode = reverseBits(code, codeBitPosition);

        //check if code exists in  code table for literals and distances
        if (const auto literalsCodeTableIterator = std::ranges::find_if(literalsCodeTable, findCodeInCodeTable); (literalsCodeTableIterator != literalsCodeTable.end()) && (!isNextDistance))
        {
            if (literalsCodeTableIterator->second < 256)
            {
                matches.emplace_back(std::byte{static_cast<std::uint8_t>(literalsCodeTableIterator->second)}, 0, 1);
                resetCode();
            }
            else if (literalsCodeTableIterator->second == 256)
            {
                isEndOfBlock = true;
            }
            else if (auto lengthOptional = tryDecodeLength(literalsCodeTableIterator->second); lengthOptional.has_value())
            {
                length = lengthOptional.value();
                isNextDistance = true;
                resetCode();
            }
        }

        if (isNextDistance)
        {
            if (auto distanceOptional = tryDecodeDistance(reversedCode, codeBitPosition); distanceOptional.has_value())
            {
                distance = distanceOptional.value();
                isNextDistance = false;
                resetCode();
            }
        }

        if ((distance != 0) && (length != 0))
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
    return bitBuffer.getByteIndex();
}

deflate::DynamicHuffmanDecoder::DynamicHuffmanDecoder(const BitBuffer &newBitsBuffer) : bitBuffer(newBitsBuffer)
{
}