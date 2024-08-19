//
// Created by cx9ps3 on 18.08.2024.
//

#include "StaticHuffmanDecoder.hpp"
#include "StaticHuffmanEncoder.hpp"

#include <array>
#include <cassert>


void deflate::StaticHuffmanDecoder::resetCode()
{
    //reset for new code
    code = 0;
    codeBitPosition = 0;
    extraBits = 0;
}

std::optional<std::uint16_t> deflate::StaticHuffmanDecoder::tryDecodeLength()
{
    auto FIXED_LENGTHS_CODES = StaticHuffmanEncoder::initializeFixedCodesForLengths();

    auto findCode = [this](const auto &arrayCode)
    {
        return arrayCode.code == code;
    };

    auto findByExtraBits = [this](const auto &distanceCode)
    {
        return (extraBits == distanceCode.extraBits) && (code == distanceCode.code);
    };

    //find length with code
    if (auto* lengthCodesIterator = std::ranges::find_if(FIXED_LENGTHS_CODES, findCode); lengthCodesIterator != FIXED_LENGTHS_CODES.cend())
    {
        //try read extra bits if code has extra bits
        isNextDistance = true;
        if (lengthCodesIterator->extraBitsCount > 0)
        {
            extraBits = bitBuffer.readBits(lengthCodesIterator->extraBitsCount);
            lengthCodesIterator = std::ranges::find_if(FIXED_LENGTHS_CODES, findByExtraBits);
            assert(lengthCodesIterator != FIXED_LENGTHS_CODES.cend() && "Fixed length code not found!");
        }

        resetCode();
        return lengthCodesIterator->length;
    }

    return std::nullopt;
}

std::optional<std::uint16_t> deflate::StaticHuffmanDecoder::tryDecodeDistance()
{
    auto FIXED_DISTANCES_CODES = StaticHuffmanEncoder::initializeFixedCodesForDistances();

    auto findCode = [this](const auto &arrayCode)
    {
        return arrayCode.code == code;
    };

    auto findByExtraBits = [this](const auto &distanceCode)
    {
        return (extraBits == distanceCode.extraBits) && (code == distanceCode.code);
    };

    //find distance with code
    if (auto*  distanceCodeIterator = std::ranges::find_if(FIXED_DISTANCES_CODES, findCode); distanceCodeIterator != FIXED_DISTANCES_CODES.cend())
    {
        //try read extra bits if code has extra bits
        if (distanceCodeIterator->extraBitsCount > 0)
        {
            extraBits = bitBuffer.readBits(distanceCodeIterator->extraBitsCount);
            distanceCodeIterator = std::ranges::find_if(FIXED_DISTANCES_CODES, findByExtraBits);
            assert(distanceCodeIterator != FIXED_DISTANCES_CODES.cend() && "Fixed length code not found!");
        }

        resetCode();
        return distanceCodeIterator->distance;
    }

    return std::nullopt;
}

std::optional<std::byte> deflate::StaticHuffmanDecoder::tryDecodeLiteral()
{
    auto findCode = [this](const auto &arrayCode)
    {
        return arrayCode.code == code;
    };

    auto FIXED_LITERALS_CODES = StaticHuffmanEncoder::initializeFixedCodesForLiterals();
    if (const auto* const literalsCodesIterator = std::ranges::find_if(FIXED_LITERALS_CODES, findCode); literalsCodesIterator != FIXED_LITERALS_CODES.cend())
    {
        resetCode();
        return literalsCodesIterator->literal;
    }

    return std::nullopt;
}

deflate::StaticHuffmanDecoder::StaticHuffmanDecoder(const BitBuffer &newBitBuffer) : bitBuffer(newBitBuffer)
{
}

std::vector<deflate::LZ77::Match> deflate::StaticHuffmanDecoder::decodeData()
{
    bool isHeaderParsed = false;
    std::uint16_t distance{0};
    std::uint16_t length{0};
    std::vector<deflate::LZ77::Match> lz77Matches;
    while (bitBuffer.next())
    {

        //read one bit from byte
        code |=  std::to_integer<std::uint16_t>(bitBuffer.readBit() << codeBitPosition);
        ++codeBitPosition;

        //check header magic numbers
        if ((!isHeaderParsed) && (codeBitPosition == 3) && ((code == 0b011) || (code == 0b010)))
        {
            isHeaderParsed = true;
            resetCode();
        }

        //check if code is literal or encoded lz77 length with code length = 8
        //code length with 8 - literals from 0 to 143 or lz77 length from 280 to 287
        //code length with 9 - literals from 144 to 255
        //see - https://datatracker.ietf.org/doc/html/rfc1951#page-12 for details

        if ((codeBitPosition == 8) || (codeBitPosition == 9))
        {
            if(auto literal = tryDecodeLiteral(); literal.has_value())
            {
                //adding current decoded literal
                lz77Matches.emplace_back(literal.value(), 0, 1);
            }

            if (codeBitPosition == 9)
            {
                resetCode();
            }
        }
        //check if code is  lz77 length with code length = 7
        //see - https://datatracker.ietf.org/doc/html/rfc1951#page-12 for details
        else if (codeBitPosition == 7)
        {
            length = tryDecodeLength().value_or(0);
        }
        //check if code is  lz77 distance with code length = 5
        //see - https://datatracker.ietf.org/doc/html/rfc1951#page-12 for details
        else if ((codeBitPosition == 5) && isNextDistance)
        {
            isNextDistance = false;
            distance = tryDecodeDistance().value_or(0);
        }

        //adding current decoded length and distance
        else if ((distance != 0) && (length != 0))
        {
            lz77Matches.emplace_back(std::byte{0}, distance, length);
            distance = 0;
            length = 0;
        }
    }

    return lz77Matches;
}