//
// Created by cx9ps3 on 01.08.2024.
//

#include "LZ77.hpp"

#include <algorithm>
#include <array>
#include <stdexcept>

namespace
{
    constexpr std::uint32_t HASH_BITS = 15;
    constexpr std::uint32_t HASH_SIZE = 1U << HASH_BITS;
    constexpr std::int32_t NO_POSITION = -1;

    /// Three bytes into a table slot.  Multiplying spreads the three bytes
    /// over the whole hash rather than letting the first dominate.
    std::uint32_t hashAt(const std::vector<std::byte> &data, const std::size_t position)
    {
        const auto value = (static_cast<std::uint32_t>(data[position]) << 16U) | (static_cast<std::uint32_t>(data[position + 1]) << 8U) | static_cast<std::uint32_t>(data[position + 2]);
        return (value * 2654435761U) >> (32U - HASH_BITS);
    }

    /// How many bytes from `earlier` and `position` on are the same, up to the most a match may say.
    std::uint16_t matchLength(const std::vector<std::byte> &data, const std::size_t earlier, const std::size_t position)
    {
        const auto limit = std::min<std::size_t>(deflate::LZ77::MAX_MATCH_LENGTH, data.size() - position);
        std::size_t length = 0;
        while (length < limit && data[earlier + length] == data[position + length])
        {
            ++length;
        }
        return static_cast<std::uint16_t>(length);
    }

    struct Chains
    {
        std::array<std::int32_t, HASH_SIZE> head{};
        std::vector<std::int32_t> previous;

        explicit Chains(const std::size_t size) : previous(size, NO_POSITION)
        {
            head.fill(NO_POSITION);
        }

        void insert(const std::vector<std::byte> &data, const std::size_t position)
        {
            if (position + deflate::LZ77::MIN_MATCH_LENGTH > data.size())
            {
                return;
            }
            const auto slot = hashAt(data, position);
            previous[position] = head[slot];
            head[slot] = static_cast<std::int32_t>(position);
        }

        /// The longest match for `position` among the earlier positions on its chain.
        [[nodiscard]] deflate::LZ77::Match longest(const std::vector<std::byte> &data, const std::size_t position, const std::uint32_t depth) const
        {
            deflate::LZ77::Match best{std::byte{0}, 0, 0};
            if (position + deflate::LZ77::MIN_MATCH_LENGTH > data.size())
            {
                return best;
            }
            const auto longestPossible = std::min<std::size_t>(deflate::LZ77::MAX_MATCH_LENGTH, data.size() - position);
            auto candidate = head[hashAt(data, position)];
            for (std::uint32_t step = 0; step < depth && candidate != NO_POSITION && best.length < longestPossible; ++step)
            {
                const auto earlier = static_cast<std::size_t>(candidate);
                if (position - earlier > deflate::LZ77::WINDOW_SIZE)
                {
                    break;
                }
                // The cheapest test first: a match that could beat the best
                // must agree where the best ends.
                if (data[earlier + best.length] == data[position + best.length])
                {
                    const auto length = matchLength(data, earlier, position);
                    if (length > best.length)
                    {
                        best.length = length;
                        best.distance = static_cast<std::uint16_t>(position - earlier);
                    }
                }
                candidate = previous[earlier];
            }
            if (best.length < deflate::LZ77::MIN_MATCH_LENGTH)
            {
                best.length = 0;
            }
            return best;
        }
    };
}// namespace

std::vector<deflate::LZ77::Match> deflate::LZ77::compress(const std::vector<std::byte> &dataToCompress, const bool thorough)
{
    std::vector<Match> compressedData;
    compressedData.reserve(dataToCompress.size() / 2);
    const std::uint32_t depth = thorough ? 1024 : 64;

    // A position is looked up before it is put in the chains, or it would
    // find itself, a distance of nothing.
    Chains chains(dataToCompress.size());
    std::size_t position = 0;
    while (position < dataToCompress.size())
    {
        auto match = chains.longest(dataToCompress, position, depth);
        chains.insert(dataToCompress, position);
        if (match.length == 0)
        {
            compressedData.push_back(Match{dataToCompress[position], 0, 1});
            ++position;
            continue;
        }

        // Lazy: a longer match one byte on is worth a literal to reach.
        std::size_t inserted = position + 1;
        if (match.length < MAX_MATCH_LENGTH && position + 1 < dataToCompress.size())
        {
            const auto next = chains.longest(dataToCompress, position + 1, depth);
            chains.insert(dataToCompress, position + 1);
            inserted = position + 2;
            if (next.length > match.length)
            {
                compressedData.push_back(Match{dataToCompress[position], 0, 1});
                ++position;
                match = next;
            }
        }

        compressedData.push_back(match);
        for (std::size_t inside = inserted; inside < position + match.length; ++inside)
        {
            chains.insert(dataToCompress, inside);
        }
        position += match.length;
    }

    return compressedData;
}

std::vector<std::byte> deflate::LZ77::decompress(const std::vector<Match> &compressedData)
{
    std::vector<std::byte> decompressedData;
    decompressedData.reserve(compressedData.size() * 2);
    decompress(compressedData, decompressedData);
    return decompressedData;
}

void deflate::LZ77::decompress(const std::vector<Match> &compressedData, std::vector<std::byte> &decompressedData)
{
    for (const Match &match: compressedData)
    {
        if (match.length == 1)
        {
            decompressedData.push_back(match.literal);
        }
        else
        {
            if (match.distance == 0 || match.distance > decompressedData.size())
            {
                throw std::runtime_error("LZ77: invalid back-reference distance");
            }
            const auto offset = decompressedData.size() - match.distance;
            for (std::uint16_t i = 0; i < match.length; ++i)
            {
                decompressedData.push_back(decompressedData[offset + i]);
            }
        }
    }
}
