//
// Created by cx9ps3 on 20.09.2024.
//

#pragma once
#include <vector>
namespace deflate
{
    class Deflator
    {
    private:
        [[nodiscard]] std::vector<std::byte> createUncompressedBlock(const std::vector<std::byte> &data, bool isLastBlock) const;

    public:
        [[nodiscard]] std::vector<std::byte> compress(const std::vector<std::byte> &data, bool isLastBlock) const;
        [[nodiscard]] std::vector<std::byte> operator()(const std::vector<std::byte> &data, bool isLastBlock) const;
    };
}// namespace deflate
