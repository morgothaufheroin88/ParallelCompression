//
// Created by cx9ps3 on 18.08.2024.
//

#pragma once
#include <cstdint>
#include <vector>

namespace deflate
{
    class BitBuffer
    {
    private:
        std::vector<std::byte> buffer;
        std::byte currentByte{0};
        std::uint8_t bitPosition{0};
        std::size_t byteIndex{0};

    public:
        BitBuffer() = default;
        explicit BitBuffer(const std::vector<std::byte> &newBuffer);
        std::byte readBit();
        std::uint32_t readBits(std::size_t numberOfBits);
        void writeBits(std::uint32_t value, std::size_t numberOfBits);
        [[nodiscard]] std::vector<std::byte> getBytes();
        [[nodiscard]] bool next() const noexcept;
    };
}// namespace deflate