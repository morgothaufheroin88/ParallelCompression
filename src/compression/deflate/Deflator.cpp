//
// Created by cx9ps3 on 20.09.2024.
//

#include "Deflator.hpp"

#include "../encoders/DynamicHuffmanEncoder.hpp"
#include "../encoders/FixedHuffmanEncoder.hpp"
#include "../lz/LZ77.hpp"
#include <future>

std::vector<std::byte> deflate::Deflator::createUncompressedBlock(const std::vector<std::byte> &data, const bool isLastBlock) const
{
    std::vector<std::byte> result;

    //write header to buffer
    auto blockHeader = std::byte{0x00};
    if (isLastBlock)
    {
        blockHeader = std::byte{0x10};
    }
    result.push_back(blockHeader);

    const auto len = static_cast<uint16_t>(data.size());
    const auto nlen = static_cast<std::uint16_t>(~len);

    //write LEN to buffer
    result.push_back(std::byte{static_cast<std::uint8_t>(len & 0xFF)});
    result.push_back(std::byte{static_cast<std::uint8_t>((len >> 8) & 0xFF)});

    //write NLEN to buffer
    result.push_back(std::byte{static_cast<std::uint8_t>(nlen & 0xFF)});
    result.push_back(std::byte{static_cast<std::uint8_t>((nlen >> 8) & 0xFF)});

    result.insert(result.end(), data.begin(), data.end());
    return result;
}

std::vector<std::byte> deflate::Deflator::compress(const std::vector<std::byte> &data, const bool isLastBlock, const CompressionLevel compressionLevel) const
{
    const auto isUseHashTable = (static_cast<std::int32_t>(compressionLevel) > 0);

    const auto lz77Matches = LZ77::compress(data, isUseHashTable);
    const auto fixedEncoding = [&lz77Matches, isLastBlock = isLastBlock]()
    {
        return FixedHuffmanEncoder::encodeData(lz77Matches, isLastBlock);
    };

    const auto dynamicEncoding = [&lz77Matches, isLastBlock = isLastBlock]()
    {
        DynamicHuffmanEncoder encoder;
        return encoder.encodeData(lz77Matches, isLastBlock);
    };

    auto encodedWithFixedCodesDataFuture = std::async(std::launch::async, fixedEncoding);
    auto encodedWithDynamicCodesDataFuture = std::async(std::launch::async, dynamicEncoding);

    auto encodedWithFixedCodesData = encodedWithFixedCodesDataFuture.get();
    auto encodedWithDynamicCodesData = encodedWithDynamicCodesDataFuture.get();

    if ((encodedWithFixedCodesData.size() > (data.size() + 5)) && (encodedWithDynamicCodesData.size() > (data.size() + 5)))
    {
        return createUncompressedBlock(data, isLastBlock);
    }
    else if (encodedWithFixedCodesData.size() < encodedWithDynamicCodesData.size())
    {
        return encodedWithFixedCodesData;
    }
    else if (encodedWithFixedCodesData.size() == encodedWithDynamicCodesData.size())
    {
        return encodedWithFixedCodesData;
    }
    else if (encodedWithFixedCodesData.size() > encodedWithDynamicCodesData.size())
    {
        return encodedWithDynamicCodesData;
    }
    else
    {
        return createUncompressedBlock(data, isLastBlock);
    }
}
std::vector<std::byte> deflate::Deflator::operator()(const std::vector<std::byte> &data, const bool isLastBlock, const CompressionLevel compressionLevel) const
{
    return compress(data, isLastBlock, compressionLevel);
}