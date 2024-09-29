//
// Created by cx9ps3 on 27.09.2024.
//

#include "Inflator.hpp"
#include "../decoders/DynamicHuffmanDecoder.hpp"
#include "../decoders/FixedHuffmanDecoder.hpp"
#include "../buffer/BitBuffer.hpp"
#include <cstddef>
#include <cstdint>

std::vector<std::byte> deflate::Inflator::getUncompressedBlock(const std::vector<std::byte> &data) const
{
    std::vector<std::byte> result;

    //ignore first five bytes, because its header. two bytes of len and two bytes of nlen
    result.insert(result.end(), data.begin() + 4, data.end());
    return result;
}

std::vector<std::byte> deflate::Inflator::decompress(const std::vector<std::byte> &data) const
{
    assert(data.size() > 0,"The input data is empty");
    const auto header = data[0];

    [[maybe_unused]] const auto isLastBlock = static_cast<bool>(std::byte{((1 << 1) - 1)} & (header >> 0));

    if (const auto blockType = std::to_integer<std::uint8_t>(std::byte{((1 << 2) - 1)} & (header >> 1)); blockType == 0)
    {
        return getUncompressedBlock(data);
    }
    else if (blockType == 1)
    {
        FixedHuffmanDecoder decoder(BitBuffer{data});
        return LZ77::decompress(decoder.decodeData());
    }
    else if (blockType == 2)
    {
        DynamicHuffmanDecoder decoder(BitBuffer{data});
        return LZ77::decompress(decoder.decodeData());
    }
    else
    {
        assert(false , "Unsupported block type");
    }

    return std::vector<std::byte>{};
}

std::vector<std::byte> deflate::Inflator::operator()(const std::vector<std::byte> &data) const
{
    return decompress(data);
}