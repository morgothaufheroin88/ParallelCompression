//
// Created by cx9ps3 on 01.10.2024.
//

#pragma once
#include <cstdint>
#include <future>
#include <vector>

namespace parallel
{
    class ParallelDecompression
    {
    private:
        using Block = std::vector<std::byte>;
        struct CompressionJob
        {
            std::uint16_t id;
            std::future<Block> compressedBlockFuture;
        };

        std::vector<Block> compressedBlocks;
        std::vector<std::pair<std::uint16_t, Block>> decompressedBlocks;

        void splitDataIntoBlocks(const std::vector<std::byte>& data);
        void createParallelJobs();
    public:
        std::vector<std::byte> decompress(const std::vector<std::byte> &data);
    };
}// namespace parallel
