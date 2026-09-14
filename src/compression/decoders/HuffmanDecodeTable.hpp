//
// A canonical Huffman code as a lookup table, for decoding.
//

#pragma once
#include "../buffer/BitBuffer.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace deflate
{
    /**
     * @brief A Huffman code turned into a table indexed by the next bits of the stream.
     *
     * DEFLATE packs a code into the stream starting with its most significant
     * bit, and the stream is read least significant bit first, so the bits a
     * reader peeks are the code reversed.  The table is indexed by exactly
     * that: the next N bits as the reader sees them, where N is the longest
     * code in the alphabet.  Every code shorter than N fills all the entries
     * whose low bits are the code, so one lookup decodes any symbol, and the
     * entry says how many of the bits it used.
     *
     * A code of fifteen bits makes a table of 32768 entries; a dynamic block's
     * literal code rarely goes past twelve or so, and the table is sized to
     * the longest code actually present, not to the maximum the format allows.
     */
    class HuffmanDecodeTable
    {
    public:
        struct Entry
        {
            std::uint16_t symbol{0};
            std::uint8_t length{0};///< Zero: no code has these bits, which a well-formed stream never produces.
        };

        HuffmanDecodeTable() = default;

        /**
         * @param codeLengths The length of each symbol's code, by symbol; zero
         *        for a symbol the code does not use.  The codes themselves
         *        follow from the lengths, as RFC 1951 3.2.2 has it.
         */
        explicit HuffmanDecodeTable(std::span<const std::uint8_t> codeLengths);

        /**
         * @brief Read one symbol from the stream.
         *
         * Fails the way the library fails on any malformed stream, rather than
         * returning a symbol that was never coded.
         */
        [[nodiscard]] std::uint16_t decode(BitBuffer &bitBuffer) const;

        /** @brief How many bits the table looks ahead: the longest code. */
        [[nodiscard]] std::uint8_t peekWidth() const noexcept;

        [[nodiscard]] bool empty() const noexcept;

    private:
        std::vector<Entry> entries;
        std::uint8_t width{0};
    };

    /**
     * @brief Read one block, up to and including its end-of-block code, onto the end of `output`.
     *
     * The same for a fixed block and a dynamic one; only the two codes differ.
     * A length symbol is followed by its extra bits, a distance code and its
     * extra bits, as RFC 1951 3.2.5 lays them out; a literal goes straight
     * into the output and a back-reference is copied from what is already
     * there, which may be as recent as the byte before -- so byte by byte,
     * never as one move.  Whatever `output` held before the call is the
     * window a back-reference may reach into, as the blocks before this one
     * are meant to be.
     */
    void decodeBlock(BitBuffer &bitBuffer, const HuffmanDecodeTable &literals, const HuffmanDecodeTable &distances, std::vector<std::byte> &output);
}// namespace deflate
