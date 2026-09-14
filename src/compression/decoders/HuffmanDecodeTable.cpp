//
// A canonical Huffman code as a lookup table, for decoding.
//

#include "HuffmanDecodeTable.hpp"

#include <algorithm>
#include <array>

namespace
{
    constexpr std::uint8_t MAX_CODE_LENGTH = 15;
    constexpr std::uint16_t END_OF_BLOCK = 256;
    constexpr std::uint16_t FIRST_LENGTH_SYMBOL = 257;

    // RFC 1951 3.2.5: what each length and distance symbol starts from, and
    // how many extra bits follow it.
    constexpr std::array<std::uint16_t, 29> LENGTH_BASE = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    constexpr std::array<std::uint8_t, 29> LENGTH_EXTRA = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    constexpr std::array<std::uint16_t, 30> DISTANCE_BASE = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    constexpr std::array<std::uint8_t, 30> DISTANCE_EXTRA = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    /// The stream shows a code back to front, so the table is indexed back to front.
    std::uint32_t reverseBits(std::uint32_t code, const std::uint8_t length)
    {
        std::uint32_t reversed = 0;
        for (std::uint8_t bit = 0; bit < length; ++bit)
        {
            reversed = (reversed << 1U) | (code & 1U);
            code >>= 1U;
        }
        return reversed;
    }
}// namespace

deflate::HuffmanDecodeTable::HuffmanDecodeTable(const std::span<const std::uint8_t> codeLengths)
{
    std::array<std::uint16_t, MAX_CODE_LENGTH + 1> countOfLength{};
    for (const auto length: codeLengths)
    {
        assert(length <= MAX_CODE_LENGTH, "Huffman code longer than DEFLATE allows");
        ++countOfLength[length];
        width = std::max(width, length);
    }
    if (width == 0)
    {
        return;
    }

    // The first code of each length, from the counts: RFC 1951 3.2.2.
    std::array<std::uint32_t, MAX_CODE_LENGTH + 2> nextCode{};
    countOfLength[0] = 0;
    std::uint32_t code = 0;
    for (std::uint8_t length = 1; length <= MAX_CODE_LENGTH; ++length)
    {
        code = (code + countOfLength[length - 1]) << 1U;
        nextCode[length] = code;
    }

    entries.assign(std::size_t{1} << width, Entry{});
    const std::uint32_t tableSize = 1U << width;
    std::size_t codesInAll = 0;
    for (std::uint16_t symbol = 0; symbol < codeLengths.size(); ++symbol)
    {
        const auto length = codeLengths[symbol];
        if (length == 0)
        {
            continue;
        }
        ++codesInAll;
        const auto reversed = reverseBits(nextCode[length]++, length);
        // Every index whose low `length` bits are this code decodes to it,
        // whatever the bits above -- those belong to the next symbol.
        for (std::uint32_t index = reversed; index < tableSize; index += (1U << length))
        {
            entries[index] = Entry{symbol, length};
        }
    }

    // A code with one symbol in it -- a block whose every back-reference is
    // the same distance -- is one bit that can only be 0.  The encoder this
    // library had before the lookup tables wrote a 1 there, and every archive it
    // made still says so; a 1 is read as the symbol too, which a stream
    // written to the letter of the format never contains.
    if (codesInAll == 1 && width == 1)
    {
        entries[1] = entries[0];
    }
}

std::uint16_t deflate::HuffmanDecodeTable::decode(BitBuffer &bitBuffer) const
{
    assert(width > 0, "Decoding with an empty Huffman code");
    const auto &entry = entries[bitBuffer.peekBits(width)];
    assert(entry.length > 0, "Bits that are no Huffman code in this block");
    bitBuffer.consumeBits(entry.length);
    return entry.symbol;
}

std::uint8_t deflate::HuffmanDecodeTable::peekWidth() const noexcept
{
    return width;
}

bool deflate::HuffmanDecodeTable::empty() const noexcept
{
    return width == 0;
}

void deflate::decodeBlock(BitBuffer &bitBuffer, const HuffmanDecodeTable &literals, const HuffmanDecodeTable &distances, std::vector<std::byte> &output)
{
    while (true)
    {
        const auto symbol = literals.decode(bitBuffer);
        if (symbol < END_OF_BLOCK)
        {
            output.push_back(std::byte{static_cast<std::uint8_t>(symbol)});
            continue;
        }
        if (symbol == END_OF_BLOCK)
        {
            return;
        }

        const auto lengthIndex = static_cast<std::size_t>(symbol - FIRST_LENGTH_SYMBOL);
        assert(lengthIndex < LENGTH_BASE.size(), "Length symbol outside the DEFLATE alphabet");
        const std::size_t length = LENGTH_BASE[lengthIndex] + bitBuffer.readBits(LENGTH_EXTRA[lengthIndex]);

        const auto distanceSymbol = distances.decode(bitBuffer);
        assert(distanceSymbol < DISTANCE_BASE.size(), "Distance symbol outside the DEFLATE alphabet");
        const std::size_t distance = DISTANCE_BASE[distanceSymbol] + bitBuffer.readBits(DISTANCE_EXTRA[distanceSymbol]);
        assert(distance > 0 && distance <= output.size(), "A back-reference to before the start of the output");

        // Overlap is the point: a distance of one and a length of ten is a
        // byte repeated ten times, so the copy is one byte at a time and
        // reads what it has just written.
        const auto from = output.size() - distance;
        for (std::size_t index = 0; index < length; ++index)
        {
            output.push_back(output[from + index]);
        }
    }
}
