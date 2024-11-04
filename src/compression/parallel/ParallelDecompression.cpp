//
// Created by cx9ps3 on 01.10.2024.
//

#include "ParallelDecompression.hpp"
#include "../deflate/Inflator.hpp"

#include <algorithm>
#include <iostream>
#include <queue>
#include <ranges>

void parallel::ParallelDecompression::splitDataIntoBlocks(const std::vector<std::byte> &data)
{
    std::uint32_t i = 0;
    compressedBlocks.reserve(data.size());

    while ((i + 1) < data.size())
    {
        const auto hiBits = static_cast<std::uint16_t>(data[i]);
        const auto loBits = static_cast<std::uint16_t>(data[i + 1]);

        const auto length = (loBits << 8) | hiBits;
        i += 2;

        Block block;
        block.insert(block.end(), data.begin() + i, data.begin() + i + length);

        compressedBlocks.push_back(block);
        i += length;
    }
}

void parallel::ParallelDecompression::createParallelJobs()
{
    std::uint16_t jobIndex = 0;
    deflate::Inflator inflator;
    std::queue<CompressionJob> jobs;
    const auto maxThreads = std::thread::hardware_concurrency();

    for (const auto &block: compressedBlocks)
    {
        if (jobIndex >= maxThreads)
        {
            const auto result = jobs.front().compressedBlockFuture.get();
            decompressedBlocks.emplace_back(jobs.front().id, result);
            jobs.pop();
        }

        jobs.emplace(jobIndex, std::async(std::launch::async, inflator, block));
        ++jobIndex;
    }

    while (!jobs.empty())
    {
        const auto result = jobs.front().compressedBlockFuture.get();
        decompressedBlocks.emplace_back(jobs.front().id, result);
        jobs.pop();
    }
}
std::vector<std::byte> parallel::ParallelDecompression::decompress(const std::vector<std::byte> &data)
{
    splitDataIntoBlocks(data);
    createParallelJobs();
    std::vector<std::byte> result;

    std::ranges::sort(decompressedBlocks, [](const auto &left, const auto &right)
                      { return left.first < right.first; });

    for (auto &[id, decompressedBlock]: decompressedBlocks)
    {
        result.insert(result.end(), decompressedBlock.begin(), decompressedBlock.end());
    }

    return result;
}