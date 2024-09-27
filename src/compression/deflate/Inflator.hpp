//
// Created by cx9ps3 on 27.09.2024.
//

#pragma once
#include <vector>


namespace deflate
{
    class Inflator
    {
    private:
        [[nodiscard]] std::vector<std::byte> getUncompressedBlock(const std::vector<std::byte> &data) const;

    public:
        [[nodiscard]] std::vector<std::byte> decompress(const std::vector<std::byte> &data) const;
        [[nodiscard]] std::vector<std::byte> operator()(const std::vector<std::byte> &data) const;
    };
}// namespace deflate
