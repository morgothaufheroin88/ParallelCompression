//
// Created by cx9ps3 on 20.08.2024.
//

#pragma once
#include "../buffer/BitBuffer.hpp"
#include "../lz/LZ77.hpp"
#include "../tree/HuffmanTree.hpp"
#include <ranges>

namespace deflate
{
    class DynamicHuffmanEncoder
    {
    private:
        template<class T>
        inline std::uint32_t encodeCodeLength(T &&range, decltype(std::begin(range)) &it, std::uint32_t &extraBits, std::uint32_t &extraBitsLength) const
        {
            auto current = *it;

            if (auto end = std::end(std::forward<T>(range)); std::distance(it, end) >= 3)
            {
                auto oldIt = it;
                if ((current == 0) && (*(it + 1) == 0) && (*(it + 2) == 0))
                {
                    it = std::ranges::find_if(it + 3, end, [=](const std::uint32_t v)
                                              { return v != 0; });
                    auto repeat = std::distance(oldIt, it);
                    if (repeat <= 10)
                    {
                        extraBits = repeat - 3;
                        extraBitsLength = 3;
                        return 17;
                    }
                    else
                    {
                        if (repeat > 138)
                        {
                            extraBits = 127;
                            extraBitsLength = 7;
                            it = oldIt + 138;
                            return 18;
                        }
                        else
                        {
                            extraBits = repeat - 11;
                            extraBitsLength = 7;
                            return 18;
                        }
                    }
                }
                else if (it != std::begin(std::forward<T>(range)) && current == *(it - 1) && current == *(it + 1) && current == *(it + 2))
                {
                    it = std::ranges::find_if(it + 3, end, [=](const std::uint32_t v)
                                              { return v != current; });
                    auto repeat = std::distance(oldIt, it);

                    if (repeat > 6)
                    {
                        extraBits = 3;
                        extraBitsLength = 2;
                        it = oldIt + 6;
                        return 16;
                    }
                    else
                    {
                        extraBits = repeat - 3;
                        extraBitsLength = 2;
                        return 16;
                    }
                }
            }

            ++it;
            extraBits = 0;
            extraBitsLength = 0;
            return current;
        }

        [[nodiscard]] inline std::uint32_t reverseBits(std::uint32_t bits, std::uint8_t bitsCount) const;

        void encodeCodeLengths(const std::vector<std::uint8_t> &lengths);
        void encodeLZ77Matches(const std::vector<LZ77::Match> &lz77Matches);

        BitBuffer bitBuffer;
        std::vector<std::uint8_t> literalsCodeLengths;
        std::vector<std::uint8_t> distancesCodeLengths;

        constexpr static std::uint16_t LITERALS_AND_LENGTHS_ALPHABET_SIZE = 287;

    public:
        std::vector<std::byte> encodeData(const std::vector<LZ77::Match> &lz77Matches, bool isLastBlock);
    };

}// namespace deflate
