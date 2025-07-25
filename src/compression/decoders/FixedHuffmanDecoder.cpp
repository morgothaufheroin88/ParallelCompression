//
// Created by cx9ps3 on 18.08.2024.
//

#include "FixedHuffmanDecoder.hpp"
#include <array>
#include <ranges>

std::optional<std::uint16_t> deflate::FixedHuffmanDecoder::tryDecodeLength(const std::uint16_t lengthFixedCode)
{
    const auto findByIndex = [&lengthFixedCode](const auto &element)
    {
        return ((element.index + 257) == lengthFixedCode) && (element.code != 0) && (element.codeLength != 0);
    };

    if (const auto lengthCodesIterator = std::ranges::find_if(FIXED_LENGTHS_CODES, findByIndex); lengthCodesIterator != FIXED_LENGTHS_CODES.end())
    {
        std::uint32_t extraBits = 0;
        if (lengthCodesIterator->extraBitsCount > 0)
        {
            extraBits = bitBuffer->readBits(lengthCodesIterator->extraBitsCount);
        }

        return lengthCodesIterator->length + extraBits;
    }

    return std::nullopt;
}

std::optional<std::uint16_t> deflate::FixedHuffmanDecoder::tryDecodeDistance(const std::uint16_t code, const std::uint8_t codeBitPosition)
{
    const auto it = std::ranges::find_if(distancesCodeTable, [&code, &codeBitPosition](const auto &pair)
                                         { return (pair.first.code == code) && (pair.first.length == codeBitPosition); });

    std::uint16_t fixedCode = 0;
    const auto findByIndex = [&fixedCode](const auto &element)
    {
        return (element.index == fixedCode) && (element.distance != 0);
    };

    if (it != distancesCodeTable.end())
    {
        fixedCode = it->second;
        if (const auto distanceCodesIterator = std::ranges::find_if(FIXED_DISTANCES_CODES, findByIndex); distanceCodesIterator != FIXED_DISTANCES_CODES.end())
        {
            std::uint32_t extraBits = 0;
            if (distanceCodesIterator->extraBitsCount > 0)
            {
                extraBits = bitBuffer->readBits(distanceCodesIterator->extraBitsCount);
            }

            return distanceCodesIterator->distance + extraBits;
        }
    }

    return std::nullopt;
}

void deflate::FixedHuffmanDecoder::decodeHeader()
{
    //check if it is a last block
    [[maybe_unused]] const auto isLastBlock = bitBuffer->readBits(1);

    //check block type
    const auto blockType = bitBuffer->readBits(2);
    assert(blockType == 1, "Wrong block type!");
}

deflate::FixedHuffmanDecoder::FixedHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitBuffer) : bitBuffer(newBitBuffer)
{

    for (const auto &fixedCode: FIXED_LITERALS_CODES)
    {
        CodeTable::CanonicalHuffmanCode huffmanCode;
        huffmanCode.code = fixedCode.code;
        huffmanCode.length = fixedCode.codeLength;
        literalsCodeTable[huffmanCode] = std::to_integer<std::uint16_t>(fixedCode.literal);
    }

    //add end of block code in table
    CodeTable::CanonicalHuffmanCode endBlockCode;
    endBlockCode.code = FIXED_LITERALS_CODES[256].code;
    endBlockCode.length = FIXED_LITERALS_CODES[256].codeLength;
    literalsCodeTable[endBlockCode] = 256;

    for (const auto &fixedCode: FIXED_LENGTHS_CODES)
    {
        CodeTable::CanonicalHuffmanCode huffmanCode;
        huffmanCode.code = fixedCode.code;
        huffmanCode.length = fixedCode.codeLength;

        if ((!literalsCodeTable.contains(huffmanCode)) && (huffmanCode.code > 0))
        {

            literalsCodeTable[huffmanCode] = fixedCode.index + 257;
        }
    }

    for (const auto &fixedCode: FIXED_DISTANCES_CODES)
    {
        CodeTable::CanonicalHuffmanCode huffmanCode;
        huffmanCode.code = fixedCode.code;
        huffmanCode.length = 5;
        if (!distancesCodeTable.contains(huffmanCode))
        {
            distancesCodeTable[huffmanCode] = fixedCode.index;
        }
    }


    decodeHeader();
}

std::vector<deflate::LZ77::Match> deflate::FixedHuffmanDecoder::decodeData()
{
    std::uint16_t distance{0};
    std::uint16_t length{0};
    std::uint16_t code{0};
    std::uint8_t codeBitPosition{0};
    std::vector<deflate::LZ77::Match> lz77Matches;
    bool isEndOfBlock{false};

    const auto findCodeInCodeTable = [&code, &codeBitPosition](const auto &pair)
    { return (pair.first.code == code) && (pair.first.length == codeBitPosition); ; };

    const auto resetCode = [&code, &codeBitPosition]()
    {
        code = 0;
        codeBitPosition = 0;
    };

    while ((!isEndOfBlock))
    {
        //read one bit from byte
        const auto bit = bitBuffer->readBit();
        code |= static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(bit) << codeBitPosition);
        ++codeBitPosition;

        //check if code exists in  code table for literals and distances
        if (const auto literalsCodeTableIterator = std::ranges::find_if(literalsCodeTable, findCodeInCodeTable); (literalsCodeTableIterator != literalsCodeTable.end()) && (!isNextDistance))
        {
            if (literalsCodeTableIterator->second < 256)
            {
                lz77Matches.emplace_back(std::byte{static_cast<std::uint8_t>(literalsCodeTableIterator->second)}, 0, 1);
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
            if (auto distanceOptional = tryDecodeDistance(code, codeBitPosition); distanceOptional.has_value())
            {
                distance = distanceOptional.value();
                isNextDistance = false;
                resetCode();
            }
        }

        if ((distance != 0) && (length != 0))
        {
            lz77Matches.emplace_back(std::byte{0}, distance, length);
            distance = 0;
            length = 0;
        }
    }

    return lz77Matches;
}
std::size_t deflate::FixedHuffmanDecoder::getBlockSize() const noexcept
{
    return bitBuffer->getByteIndex();
}