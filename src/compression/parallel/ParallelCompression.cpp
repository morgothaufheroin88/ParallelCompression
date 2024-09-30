//
// Created by cx9ps3 on 29.09.2024.
//

#include "ParallelCompression.hpp"
#include "../deflate/Deflator.hpp"
#include <algorithm>
#include <iterator>
#include <queue>
#include <ranges>

void parallel::ParallelCompression::splitDataToBlocks(const std::vector<std::byte> &data)
{
    std::uint16_t blockIndex = 0;
    std::uint16_t previousBlockSize = 0;
    std::uint32_t proceededBytes = 0;
    auto remainingBytes = data.size();

    const auto countOfBlocks = (data.size() / MAX_BLOCK_SIZE) + 1;

    blocks.resize(countOfBlocks);
    compressedBlocks.reserve(countOfBlocks);

    for (auto &block: blocks)
    {
        block.reserve(MAX_BLOCK_SIZE);
    }

    while (remainingBytes > 0)
    {
        const auto blockSize = std::min(MAX_BLOCK_SIZE, remainingBytes);

        proceededBytes += blockSize;

        Block block;
        std::ranges::copy(data.begin() + (blockIndex * previousBlockSize), data.begin() + proceededBytes, std::back_inserter(block));

        remainingBytes -= blockSize;
        previousBlockSize = static_cast<std::uint16_t>(blockSize);

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
    const auto maxThreads = std::thread::hardware_concurrency();

    for (const auto &block: blocks)
    {
        if (jobIndex >= maxThreads)
        {
            const auto result = jobs.front().compressedBlockFuture.get();
            compressedBlocks.emplace_back(jobs.front().id, result);
            jobs.pop();
        }
        isLastBlock = jobIndex == (block.size() - 1);
        jobs.emplace(jobIndex, std::async(std::launch::async, deflator, block, isLastBlock));
        ++jobIndex;
    }

    while (!jobs.empty())
    {
        const auto result = jobs.front().compressedBlockFuture.get();
        compressedBlocks.emplace_back(jobs.front().id, result);
        jobs.pop();
    }
}

std::vector<std::byte> parallel::ParallelCompression::compress(const std::vector<std::byte> &data)
{
    splitDataToBlocks(data);
    createParallelJobs();
    std::vector<std::byte> result;

    std::ranges::sort(compressedBlocks, [](const auto &left, const auto &right)
                      { return left.first < right.first; });

    for (auto &[id, compressedBlock]: compressedBlocks)
    {
        const auto compressedSize = static_cast<std::uint16_t>(compressedBlock.size());

        result.push_back(std::byte{static_cast<std::uint8_t>(compressedSize & 0xFF)});
        result.push_back(std::byte{static_cast<std::uint8_t>((compressedSize >> 8) & 0xFF)});

        result.insert(result.end(), compressedBlock.begin(), compressedBlock.end());
    }
    return result;
}