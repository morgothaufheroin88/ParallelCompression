//
// Created by cx9ps3 on 18.08.2024.
//

#include "BitBuffer.hpp"
#include <cstddef>

deflate::BitBuffer::BitBuffer(const std::vector<std::byte> &newBuffer) : buffer(newBuffer)
{
    assert(!newBuffer.empty(), "Buffer is empty!");
    currentByte = newBuffer[0];
}

void deflate::BitBuffer::alignToByte()
{
    if (byteIndex != 0)
    {
        bitPosition = 0;
        ++byteIndex;
    }
}


std::byte deflate::BitBuffer::readBit()
{
    if (bitsAvailable == 0)
    {
        if (byteIndex < buffer.size())
        {
            bitCache = static_cast<std::uint64_t>(std::to_integer<uint8_t>(buffer[byteIndex])) << bitsAvailable;
            bitsAvailable += 8;
            ++byteIndex;
        }
    }

    const auto bit = std::byte{static_cast<uint8_t>(bitCache & 1)};
    bitCache >>= 1;
    --bitsAvailable;
    return bit;
}

std::uint32_t deflate::BitBuffer::readBits(const std::size_t numberOfBits)
{
    while (bitsAvailable < numberOfBits)
    {
        if (byteIndex < buffer.size())
        {
            bitCache |= (static_cast<uint64_t>(std::to_integer<uint8_t>(buffer[byteIndex])) << bitsAvailable);
            bitsAvailable += 8;
            ++byteIndex;
        }
        else
        {
            break;
        }
    }

    const auto result = static_cast<uint32_t>(bitCache & ((1ull << numberOfBits) - 1));
    bitCache >>= numberOfBits;
    bitsAvailable -= numberOfBits;
    return result;
}

void deflate::BitBuffer::writeBits(std::uint32_t value, std::size_t numberOfBits)
{
    while (numberOfBits > 0)
    {
        const auto bitsToWrite = std::min(numberOfBits, static_cast<std::size_t>(8 - bitPosition));
        const auto extractedBits = value & ((1 << bitsToWrite) - 1);
        const auto bits = static_cast<std::byte>(extractedBits << bitPosition);
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

std::size_t deflate::BitBuffer::getByteIndex() const noexcept
{
    return byteIndex;
}

std::uint32_t deflate::BitBuffer::peekBits(std::uint32_t numberOfBits)
{
    const auto savedByteIndex = byteIndex;
    const auto savedBitPosition = bitPosition;
    const auto savedCurrentByte = currentByte;
    const auto value = readBits(numberOfBits);

    bitPosition = savedBitPosition;
    currentByte = savedCurrentByte;
    byteIndex = savedByteIndex;

    return value;
}