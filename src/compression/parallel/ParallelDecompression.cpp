//
// Created by cx9ps3 on 01.10.2024.
//

#include "ParallelDecompression.hpp"

#include "../buffer/BitBuffer.hpp"
#include "../deflate/Inflator.hpp"
#include "CRC32.hpp"

#include <algorithm>
#include <format>
#include <iostream>
#include <queue>
#include <ranges>

void parallel::ParallelDecompression::splitDataIntoBlocks(const std::vector<std::byte> &data)
{
    std::uint32_t i = 0;
    compressedBlocks.reserve(data.size());

    while ((i + 6) < data.size())
    {
        const auto hiBits = static_cast<std::uint16_t>(data[i]);
        const auto loBits = static_cast<std::uint16_t>(data[i + 1]);

        const auto hash = (static_cast<std::uint32_t>(data[i + 2]) << 24) | (static_cast<std::uint32_t>(data[i + 3]) << 16) | (static_cast<std::uint32_t>(data[i + 4]) << 8) | static_cast<std::uint32_t>(data[i + 5]);

        const auto length = static_cast<std::uint16_t>(loBits << 8) | hiBits;
        i += 6;

        Block block;
        block.insert(block.end(), data.begin() + i, data.begin() + i + length);
        hashes.push_back(hash);
        compressedBlocks.push_back(block);
        i += static_cast<std::uint32_t>(length);
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
    std::uint32_t hashIndex = 0;
    std::ranges::sort(decompressedBlocks, [](const auto &left, const auto &right)
                      { return left.first < right.first; });

    for (auto &[id, decompressedBlock]: decompressedBlocks)
    {
        const auto hashToCompare = deflate::crc32(decompressedBlock);
        deflate::assert(hashToCompare == hashes[hashIndex], std::format("Hash of block {} is different!", id));
        result.insert(result.end(), decompressedBlock.begin(), decompressedBlock.end());
        ++hashIndex;
    }

    return result;
}