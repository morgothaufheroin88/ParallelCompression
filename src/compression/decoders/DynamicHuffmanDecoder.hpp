//
// Created by cx9ps3 on 26.08.2024.
//

#pragma once
#include "../buffer/BitBuffer.hpp"
#include "../encoders/FixedHuffmanEncoder.hpp"
#include "../lz/LZ77.hpp"
#include "../tree/HuffmanTree.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace deflate
{
    class DynamicHuffmanDecoder
    {
    private:
        std::shared_ptr<BitBuffer> bitBuffer{nullptr};
        void decodeHeader();
        std::vector<std::uint8_t> decodeCCL();
        void decodeCodeLengths();
        std::vector<LZ77::Match> decodeBody();
        [[nodiscard]] inline std::uint32_t reverseBits(std::uint32_t bits, std::uint8_t bitsCount) const;
        [[nodiscard]] inline std::optional<std::uint16_t> tryDecodeLength(std::uint16_t lengthFixedCode);
        [[nodiscard]] inline std::optional<std::uint16_t> tryDecodeDistance(std::uint32_t code, std::uint8_t codeBitPosition);
        [[nodiscard]] inline std::optional<std::uint16_t> tryDecodeDistance(std::uint16_t symbol);

        std::vector<std::uint8_t> literalsCodeLengths;
        std::vector<std::uint8_t> distanceCodeLengths;
        std::uint8_t HLIT{0};
        std::uint8_t HDIST{0};
        std::uint8_t HCLEN{0};
        bool isNextDistance = false;
        CodeTable::ReverseHuffmanCodeTable literalsCodeTable;
        CodeTable::ReverseHuffmanCodeTable distancesCodeTable;

        static constexpr std::array<std::uint16_t, 29> LENGTH_BASE = {
                3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31,
                35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};

        static constexpr std::array<std::uint8_t, 29> LENGTH_EXTRA = {
                0, 0, 0, 0, 0, 0, 0, 0,
                1, 1, 1, 1,
                2, 2, 2, 2,
                3, 3, 3, 3,
                4, 4, 4, 4,
                5, 5, 5, 5,
                0};

        static constexpr std::array<std::uint16_t, 30> DISTANCE_BASE = {
                1, 2, 3, 4, 5, 7, 9, 13,
                17, 25, 33, 49, 65, 97,
                129, 193, 257, 385, 513, 769,
                1025, 1537, 2049, 3073,
                4097, 6145, 8193, 12289,
                16385, 24577};

        static constexpr std::array<std::uint8_t, 30> DISTANCE_EXTRA = {
                0, 0, 0, 0, 1, 1, 2, 2,
                3, 3, 4, 4, 5, 5,
                6, 6, 7, 7, 8, 8,
                9, 9, 10, 10,
                11, 11, 12, 12,
                13, 13};

    public:
        explicit DynamicHuffmanDecoder(const std::shared_ptr<BitBuffer> &newBitsBuffer);
        std::vector<LZ77::Match> decodeData();
        [[nodiscard]] std::size_t getBlockSize() const noexcept;
    };
}// namespace deflate
