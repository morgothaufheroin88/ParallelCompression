//
// Created by cx9ps3 on 18.08.2024.
//

#pragma once
#include <cstdint>
#include <iostream>
#include <source_location>
#include <string_view>
#include <vector>

namespace deflate
{
    static inline void assert(const bool condition, const std::string_view &message)
    {
        if (!condition)
        {
            const std::source_location location = std::source_location::current();

            std::cerr << "Assertion failed: " << message << " at " << location.file_name() << '('
                      << location.line() << ':'
                      << location.column() << ") `"
                      << location.function_name() << "\n";
        }
    }
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