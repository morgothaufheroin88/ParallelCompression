//
// Created by cx9ps3 on 26.08.2024.
//

#include "DynamicHuffmanDecoder.hpp"

#include <array>

void deflate::DynamicHuffmanDecoder::decodeHeader()
{
    const auto literalCount = static_cast<std::uint16_t>(bitBuffer->readBits(5) + 257);
    const auto distanceCount = static_cast<std::uint16_t>(bitBuffer->readBits(5) + 1);
    const auto codeLengthCount = static_cast<std::uint16_t>(bitBuffer->readBits(4) + 4);

    // The code the lengths are written in: three bits each, in the order
    // the RFC gives, which puts the likeliest first so the rest can be left out.
    constexpr std::array<std::uint8_t, 19> ORDER = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
    std::array<std::uint8_t, 19> codeLengthLengths{};
    for (std::uint16_t index = 0; index < codeLengthCount; ++index)
    {
        codeLengthLengths[ORDER[index]] = static_cast<std::uint8_t>(bitBuffer->readBits(3));
    }
    const HuffmanDecodeTable codeLengthCode{codeLengthLengths};

    // Literal and distance lengths come as one run, and a symbol may repeat
    // the previous length or write a run of zeros across the boundary.
    std::vector<std::uint8_t> lengths;
    lengths.reserve(static_cast<std::size_t>(literalCount) + distanceCount);
    while (lengths.size() < static_cast<std::size_t>(literalCount) + distanceCount)
    {
        const auto symbol = codeLengthCode.decode(*bitBuffer);
        if (symbol < 16)
        {
            lengths.push_back(static_cast<std::uint8_t>(symbol));
        }
        else if (symbol == 16)
        {
            assert(!lengths.empty(), "A repeat with nothing before it to repeat");
            const auto previous = lengths.back();
            lengths.insert(lengths.end(), bitBuffer->readBits(2) + 3, previous);
        }
        else if (symbol == 17)
        {
            lengths.insert(lengths.end(), bitBuffer->readBits(3) + 3, std::uint8_t{0});
        }
        else
        {
            lengths.insert(lengths.end(), bitBuffer->readBits(7) + 11, std::uint8_t{0});
        }
    }
    assert(lengths.size() == static_cast<std::size_t>(literalCount) + distanceCount, "A run of code lengths past the end of the header");

    const std::span<const std::uint8_t> all{lengths};
    literals = HuffmanDecodeTable{all.first(literalCount)};
    distances = HuffmanDecodeTable{all.subspan(literalCount)};
}

void deflate::DynamicHuffmanDecoder::decodeData(std::vector<std::byte> &output)
{
    decodeHeader();
    decodeBlock(*bitBuffer, literals, distances, output);
}

std::size_t deflate::DynamicHuffmanDecoder::getBlockSize() const noexcept
{
    return bitBuffer->getByteIndex();
}

deflate::DynamicHuffmanDecoder::DynamicHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitsBuffer) : bitBuffer(newBitsBuffer)
{
}
