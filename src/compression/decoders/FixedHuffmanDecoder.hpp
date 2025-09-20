//
// Created by cx9ps3 on 18.08.2024.
//

#pragma once
#include "../buffer/BitBuffer.hpp"
#include "../encoders/FixedHuffmanEncoder.hpp"
#include "../lz/LZ77.hpp"
#include "../tree/HuffmanTree.hpp"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace deflate
{
    class BitBuffer;
    class FixedHuffmanDecoder
    {
    private:
        std::shared_ptr<BitBuffer> bitBuffer{nullptr};
        bool isNextDistance = false;
        CodeTable::ReverseHuffmanCodeTable literalsCodeTable;
        CodeTable::ReverseHuffmanCodeTable distancesCodeTable;

        std::optional<std::uint16_t> tryDecodeLength(std::uint16_t lengthFixedCode);
        std::optional<std::uint16_t> tryDecodeDistance(std::uint16_t code, std::uint8_t codeBitPosition);

    public:
        explicit FixedHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitBuffer);
        std::vector<LZ77::Match> decodeData();
        [[nodiscard]] std::size_t getBlockSize() const noexcept;
    };
}// namespace deflate
