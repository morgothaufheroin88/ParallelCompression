//
// Created by cx9ps3 on 27.09.2024.
//

#include "Inflator.hpp"
#include "../buffer/BitBuffer.hpp"
#include "../decoders/DynamicHuffmanDecoder.hpp"
#include "../decoders/FixedHuffmanDecoder.hpp"

#include <cstddef>
#include <cstdint>
#include <format>

std::vector<std::byte> deflate::Inflator::getUncompressedBlock(const std::vector<std::byte> &data) const
{
    std::vector<std::byte> result;

    //ignore first five bytes, because its header. two bytes of len and two bytes of nlen
    result.insert(result.end(), data.begin() + 5, data.end());
    return result;
}

std::vector<std::byte> deflate::Inflator::decompress(const std::vector<std::byte> &data)
{
    assert(!data.empty(), "The input data is empty");

    bitBuffer = std::make_shared<BitBuffer>(data);

    _isLastBlock = static_cast<bool>(bitBuffer->readBit());

    if (const auto blockType = bitBuffer->readBits(2); blockType == 0)
    {
        return getUncompressedBlock(data);
    }
    else if (blockType == 1)
    {
        std::vector<std::byte> result;
        FixedHuffmanDecoder decoder(bitBuffer);
        decoder.decodeData(result);
        blockSize = decoder.getBlockSize();
        return result;
    }
    else if (blockType == 2)
    {
        std::vector<std::byte> result;
        DynamicHuffmanDecoder decoder(bitBuffer);
        decoder.decodeData(result);
        blockSize = decoder.getBlockSize();
        return result;
    }
    else
    {
        assert(false, std::format("Unsupported block type : {}", blockType));
    }

    return std::vector<std::byte>{};
}

std::vector<std::byte> deflate::Inflator::operator()(const std::vector<std::byte> &data)
{
    return decompress(data);
}
std::size_t deflate::Inflator::getBlockSize() const noexcept
{
    return blockSize;
}

bool deflate::Inflator::isLastBlock() const noexcept
{
    return _isLastBlock;
}