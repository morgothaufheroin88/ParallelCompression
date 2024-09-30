//
// Created by cx9ps3 on 29.09.2024.
//

#pragma once
#include <cstdint>
#include <future>
#include <vector>

namespace parallel
{
    class ParallelCompression
    {
    private:
        using Block = std::vector<std::byte>;
        struct CompressionJob
        {
            std::uint16_t id;
            std::future<Block> compressedBlockFuture;
        };

        constexpr static std::uint64_t MAX_BLOCK_SIZE = 32 * 1024;
        std::vector<Block> blocks;
        std::vector<std::pair<std::uint16_t, Block>> compressedBlocks;


        void splitDataToBlocks(const std::vector<std::byte> &data);
        void createParallelJobs();

    public:
        std::vector<std::byte> compress(const std::vector<std::byte> &data);
    };
}// namespace parallel
