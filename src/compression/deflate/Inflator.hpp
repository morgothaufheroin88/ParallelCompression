//
// Created by cx9ps3 on 27.09.2024.
//

#pragma once
#include "../buffer/BitBuffer.hpp"


#include <memory>
#include <vector>


namespace deflate
{
    class Inflator
    {
    private:
        [[nodiscard]] std::vector<std::byte> getUncompressedBlock(const std::vector<std::byte> &data) const;
        std::size_t blockSize{0};
        std::shared_ptr<BitBuffer> bitBuffer{nullptr};
        bool _isLastBlock{false};

    public:
        [[nodiscard]] std::vector<std::byte> decompress(const std::vector<std::byte> &data);
        [[nodiscard]] std::vector<std::byte> operator()(const std::vector<std::byte> &data);
        [[nodiscard]] std::size_t getBlockSize() const noexcept;
        [[nodiscard]] bool isLastBlock() const noexcept;
    };
}// namespace deflate
