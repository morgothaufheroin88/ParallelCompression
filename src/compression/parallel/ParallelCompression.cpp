//
// Created by cx9ps3 on 29.09.2024.
//

#include "ParallelCompression.hpp"
#include "../deflate/Deflator.hpp"
#include "CRC32.hpp"
#include <algorithm>
#include <iterator>
#include <queue>
#include <ranges>

void parallel::ParallelCompression::splitDataToBlocks(const std::vector<std::byte> &data)
{
    const auto MAX_BLOCK_SIZE = static_cast<std::uint64_t>(std::abs(static_cast<std::int32_t>(compressionLevel)) * 1024);
    auto remainingBytes = data.size();

    const auto countOfBlocks = (data.size() + MAX_BLOCK_SIZE - 1) / MAX_BLOCK_SIZE;

    blocks.resize(countOfBlocks);
    compressedBlocks.reserve(countOfBlocks);

    for (auto &block: blocks)
    {
        block.reserve(MAX_BLOCK_SIZE);
    }

    std::size_t blockIndex = 0;
    std::size_t proceededBytes = 0;
    while (remainingBytes > 0)
    {
        const auto blockSize = std::min(MAX_BLOCK_SIZE, remainingBytes);
        const auto blockStart = proceededBytes;
        proceededBytes += static_cast<std::size_t>(blockSize);

        Block block;
        block.insert(block.end(), data.begin() + static_cast<std::ptrdiff_t>(blockStart), data.begin() + static_cast<std::ptrdiff_t>(proceededBytes));
        remainingBytes -= blockSize;

        blocks[blockIndex] = block;
        ++blockIndex;
    }
}

void parallel::ParallelCompression::createParallelJobs()
{
    std::uint16_t jobIndex = 0;
    bool isLastBlock = false;
    deflate::Deflator deflator;
    std::queue<CompressionJob> jobs;
    const auto maxThreads = std::max(1u, std::thread::hardware_concurrency());

    for (const auto &block: blocks)
    {
        if (jobIndex >= maxThreads)
        {
            const auto result = jobs.front().compressedBlockFuture.get();
            compressedBlocks.emplace_back(result, jobs.front().hash, jobs.front().id);
            jobs.pop();
        }
        isLastBlock = jobIndex == (blocks.size() - 1);
        const auto uncompressedBlockHash = deflate::crc32(block);
        jobs.emplace(jobIndex, uncompressedBlockHash, std::async(std::launch::async, deflator, block, isLastBlock, compressionLevel));
        ++jobIndex;
    }

    while (!jobs.empty())
    {
        const auto result = jobs.front().compressedBlockFuture.get();
        compressedBlocks.emplace_back(result, jobs.front().hash, jobs.front().id);
        jobs.pop();
    }
}

std::vector<std::byte> parallel::ParallelCompression::compress(const std::vector<std::byte> &data, const deflate::CompressionLevel newCompressionLevel)
{
    compressionLevel = newCompressionLevel;

    splitDataToBlocks(data);
    createParallelJobs();
    std::vector<std::byte> result;
    result.reserve(data.size());

    std::ranges::sort(compressedBlocks, [](const auto &left, const auto &right)
                      { return left.id < right.id; });


    for (auto &[compressedBlock, hash, id]: compressedBlocks)
    {
        const auto compressedSize = static_cast<std::uint16_t>(compressedBlock.size());

        result.push_back(std::byte{static_cast<std::uint8_t>(compressedSize & 0xFF)});
        result.push_back(std::byte{static_cast<std::uint8_t>((compressedSize >> 8) & 0xFF)});

        result.push_back(std::byte{static_cast<std::uint8_t>((hash >> 24) & 0xFF)});
        result.push_back(std::byte{static_cast<std::uint8_t>((hash >> 16) & 0xFF)});
        result.push_back(std::byte{static_cast<std::uint8_t>((hash >> 8) & 0xFF)});
        result.push_back(std::byte{static_cast<std::uint8_t>(hash & 0xFF)});

        result.insert(result.end(), compressedBlock.begin(), compressedBlock.end());
    }

    return result;
}
