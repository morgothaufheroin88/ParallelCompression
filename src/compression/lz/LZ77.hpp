//
// Created by cx9ps3 on 01.08.2024.
//

#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace deflate
{
    class LZ77
    {
    public:
        struct Match
        {
            std::byte literal;
            std::uint16_t distance;
            std::uint16_t length;
        };

        static constexpr std::uint16_t MIN_MATCH_LENGTH = 3;
        static constexpr std::uint16_t MAX_MATCH_LENGTH = 258;
        static constexpr std::uint32_t WINDOW_SIZE = 32 * 1024;

        /**
         * @brief Find the literals and back-references that make up the data.
         *
         * Every position is hashed on its first three bytes into a chain of
         * the earlier positions that hashed the same, and the longest match
         * along the chain wins.  A match is not taken at once: if the next
         * position holds a longer one, the byte is sent as a literal and the
         * longer match taken instead -- zlib's lazy evaluation, which is
         * most of what separates a fair ratio from a good one.
         *
         * @param thorough How far down each chain to look: a longer search
         *        finds longer matches and takes longer to do it.  The two
         *        settings the compression levels distinguish.
         */
        static std::vector<Match> compress(const std::vector<std::byte> &dataToCompress, bool thorough);
        static std::vector<std::byte> decompress(const std::vector<Match> &compressedData);
        static void decompress(const std::vector<Match> &compressedData, std::vector<std::byte> &decompressedData);
    };
}// namespace deflate
