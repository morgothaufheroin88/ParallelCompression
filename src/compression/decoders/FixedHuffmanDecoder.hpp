//
// Created by cx9ps3 on 18.08.2024.
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
     * @brief Decodes a block written with the fixed codes of RFC 1951 3.2.6.
     *
     * The two codes never change, so their tables are built once for the
     * program and shared by every block.
     */
    class FixedHuffmanDecoder
    {
    private:
        std::shared_ptr<BitBuffer> bitBuffer{nullptr};

        [[nodiscard]] static const HuffmanDecodeTable &literalsTable();
        [[nodiscard]] static const HuffmanDecodeTable &distancesTable();

    public:
        explicit FixedHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitBuffer);
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
