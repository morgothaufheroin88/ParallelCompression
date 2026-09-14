//
// Created by cx9ps3 on 26.08.2024.
//

#pragma once
#include "../buffer/BitBuffer.hpp"
#include "HuffmanDecodeTable.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace deflate
{
    /**
     * @brief Decodes a block that carries its own codes, RFC 1951 3.2.7.
     *
     * The header gives the lengths of the literal and distance codes,
     * themselves written with a third code whose lengths come first; each
     * is turned into a table and the body is read like a fixed block.
     */
    class DynamicHuffmanDecoder
    {
    private:
        std::shared_ptr<BitBuffer> bitBuffer{nullptr};
        HuffmanDecodeTable literals;
        HuffmanDecodeTable distances;

        void decodeHeader();

    public:
        explicit DynamicHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitsBuffer);
        /**
         * @brief Decode the block onto the end of `output`.
         *
         * What `output` already holds is the window: a back-reference in this
         * block may reach into the blocks decoded before it.
         */
        void decodeData(std::vector<std::byte> &output);
        [[nodiscard]] std::size_t getBlockSize() const noexcept;
    };
}// namespace deflate
