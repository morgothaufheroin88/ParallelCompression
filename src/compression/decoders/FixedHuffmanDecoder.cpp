//
// Created by cx9ps3 on 18.08.2024.
//

#include "FixedHuffmanDecoder.hpp"

#include <array>

const deflate::HuffmanDecodeTable &deflate::FixedHuffmanDecoder::literalsTable()
{
    // RFC 1951 3.2.6: 0..143 eight bits, 144..255 nine, 256..279 seven,
    // 280..287 eight -- the last eight symbols exist in the code and never
    // in a stream.
    static const HuffmanDecodeTable table = []
    {
        std::array<std::uint8_t, 288> lengths{};
        for (std::size_t symbol = 0; symbol < lengths.size(); ++symbol)
        {
            lengths[symbol] = symbol < 144 ? 8 : symbol < 256 ? 9 : symbol < 280 ? 7 : 8;
        }
        return HuffmanDecodeTable{lengths};
    }();
    return table;
}

const deflate::HuffmanDecodeTable &deflate::FixedHuffmanDecoder::distancesTable()
{
    // Thirty distance codes of five bits each, plus two the format reserves.
    static const HuffmanDecodeTable table = []
    {
        std::array<std::uint8_t, 32> lengths{};
        lengths.fill(5);
        return HuffmanDecodeTable{lengths};
    }();
    return table;
}

deflate::FixedHuffmanDecoder::FixedHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitBuffer) : bitBuffer(newBitBuffer)
{
}

void deflate::FixedHuffmanDecoder::decodeData(std::vector<std::byte> &output)
{
    decodeBlock(*bitBuffer, literalsTable(), distancesTable(), output);
}

std::size_t deflate::FixedHuffmanDecoder::getBlockSize() const noexcept
{
    return bitBuffer->getByteIndex();
}
