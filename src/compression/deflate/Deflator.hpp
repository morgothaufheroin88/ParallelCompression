//
// Created by cx9ps3 on 20.09.2024.
//

#pragma once
#include <vector>
namespace deflate
{
    enum class CompressionLevel
    {
        LEVEL_1 = 8,
        LEVEL_2 = 16,
        LEVEL_3 = 32,
        LEVEL_4 = -8,
        LEVEL_5 = -16,
        LEVEL_6 = -32
    };

    class Deflator
    {
    private:
        [[nodiscard]] std::vector<std::byte> createUncompressedBlock(const std::vector<std::byte> &data, bool isLastBlock) const;

    public:
        [[nodiscard]] std::vector<std::byte> compress(const std::vector<std::byte> &data, bool isLastBlock, CompressionLevel compressionLevel) const;
        [[nodiscard]] std::vector<std::byte> operator()(const std::vector<std::byte> &data, bool isLastBlock, CompressionLevel compressionLevel) const;
    };
}// namespace deflate
