//
// Created by cx9ps3 on 26.08.2024.
//

#include "DynamicHuffmanDecoder.hpp"

#include "HuffmanTree.hpp"
#include <array>
#include <bitset>
#include <cassert>
#include <iostream>

void deflate::DynamicHuffmanDecoder::decodeHeader()
{
    //read block type
    [[maybe_unused]] const auto isLastBlock = bitBuffer.readBit();
    const auto blockType = bitBuffer.readBits(2);
    assert(blockType == 0b10 && "wrong block type");

    //read count of literals in table
    HLIT = static_cast<std::uint8_t>(bitBuffer.readBits(5));

    //read count of distances in table
    HDIST = static_cast<std::uint8_t>(bitBuffer.readBits(5));

    //read count of ccls
    HCLEN = static_cast<std::uint8_t>(bitBuffer.readBits(4));

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


void deflate::DynamicHuffmanDecoder::decodeCodeLengths()
{
    const auto ccl = decodeCCL();
    const auto reverseCodeTable = deflate::CodeTable::createReverseCodeTable(ccl, 19);
    const auto codeLengthsCount = HLIT + 257 + HDIST + 1;
    std::uint16_t i{0};
    std::uint16_t code{0};
    std::uint8_t codeBitPosition{0};
    std::uint16_t previousLength{0};

    for (const auto &[code, codeLength]: reverseCodeTable)
    {
        std::cout << std::bitset<16>(code.code).to_string().substr(16 - code.length, 16) << " val = " << codeLength << std::endl;
    }

    while (i < codeLengthsCount)
    {
        //read one bit from byte
        code |= std::to_integer<std::uint16_t>(bitBuffer.readBit() << codeBitPosition);
        ++codeBitPosition;

        auto reversedBits = reverseBits(code, codeBitPosition);

        const auto it = std::ranges::find_if(reverseCodeTable, [reversedBits, codeBitPosition](const auto &pair)
                                             { return (pair.first.code == reversedBits) && (pair.first.length == codeBitPosition); });

        if (it != reverseCodeTable.end())
        {
            ++i;
            if (it->second == 18)
            {
                auto repeat = bitBuffer.readBits(7);
            }
            else if (it->second == 17)
            {
                auto repeat = bitBuffer.readBits(3);
            }
            else if ((previousLength < 16) && (previousLength > 0) && (it->second == 16))
            {
                auto repeat = bitBuffer.readBits(2);
            }

            previousLength = it->second;
            code = 0;
            codeBitPosition = 0;
        }
    }
}

std::vector<deflate::LZ77::Match> deflate::DynamicHuffmanDecoder::decodeData()
{
    std::vector<LZ77::Match> matches;
    decodeHeader();

    return matches;
}

deflate::DynamicHuffmanDecoder::DynamicHuffmanDecoder(const BitBuffer &newBitsBuffer) : bitBuffer(newBitsBuffer)
{
}