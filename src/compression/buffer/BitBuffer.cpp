//
// Created by cx9ps3 on 18.08.2024.
//

#include "BitBuffer.hpp"

deflate::BitBuffer::BitBuffer(const std::vector<std::byte> &newBuffer) : buffer(newBuffer)
{
    currentByte = newBuffer[0];
}

std::byte deflate::BitBuffer::readBit()
{
    if (buffer.empty())
    {
        return std::byte{0};
    }

    const auto bit = std::byte{static_cast<std::uint8_t>(static_cast<std::uint16_t>(currentByte) >> bitPosition & 1)};
    ++bitPosition;
    if (bitPosition == 8)
    {
        ++byteIndex;
        bitPosition = 0;
        currentByte = buffer[byteIndex];
    }
    return bit;
}

std::uint32_t deflate::BitBuffer::readBits(const std::size_t numberOfBits)
{
    std::uint32_t result = 0;
    for (std::size_t i = 0; i < numberOfBits; ++i)
    {
        result |= static_cast<std::uint32_t>(readBit() << i);
    }
    return result;
}

void deflate::BitBuffer::writeBits(std::uint32_t value, std::size_t numberOfBits)
{
    while (numberOfBits > 0)
    {
        const auto bitsToWrite = std::min(numberOfBits, static_cast<std::size_t>(8 - bitPosition));
        const auto bits = static_cast<std::byte>((value & ((1 << bitsToWrite) - 1)) << bitPosition);
        currentByte |= bits;

        bitPosition += bitsToWrite;
        numberOfBits -= bitsToWrite;
        value >>= bitsToWrite;

        if (bitPosition == 8)
        {
            buffer.push_back(currentByte);
            currentByte = std::byte{0};
            bitPosition = 0;
        }
    }
}

std::vector<std::byte> deflate::BitBuffer::getBytes()
{
    if (bitPosition > 0)
    {
        buffer.push_back(currentByte);
    }

    return buffer;
}

bool deflate::BitBuffer::next() const noexcept
{
    return byteIndex < buffer.size();
}