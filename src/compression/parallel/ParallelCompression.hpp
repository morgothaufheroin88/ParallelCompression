//
// Created by cx9ps3 on 29.09.2024.
//

#pragma once
#include "../decoders/DynamicHuffmanDecoder.hpp"
#include "../deflate/Deflator.hpp"


#include <cstdint>
#include <future>
#include <vector>

namespace deflate
{
    enum class CompressionLevel;
}

namespace parallel
{
    class ParallelCompression
    {
    private:
        using Block = std::vector<std::byte>;
        struct CompressionJob
        {
            std::uint16_t id;
            std::uint32_t hash;
            std::future<Block> compressedBlockFuture;
        };
        struct CompressedBlock
        {
            Block data;
            std::uint32_t hash;
            std::uint16_t id;
        };
        std::vector<Block> blocks;
        std::vector<CompressedBlock> compressedBlocks;
        deflate::CompressionLevel compressionLevel = deflate::CompressionLevel::LEVEL_3;

        void splitDataToBlocks(const std::vector<std::byte> &data);
        void createParallelJobs();

    public:
        std::vector<std::byte> compress(const std::vector<std::byte> &data, deflate::CompressionLevel newCompressionLevel);
    };
}// namespace parallel
