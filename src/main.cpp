#include "compression/Compression.hpp"
#include <fstream>
#include <iostream>
#include <iterator>

int main()
{

    std::ifstream input("test.bmp", std::ios::binary);

    input.seekg(0, std::ios::end);
    const auto fileSize = input.tellg();
    input.seekg(0, std::ios::beg);

    std::vector<std::byte> dataToDecompress;

    dataToDecompress.resize(fileSize);
    input.read(reinterpret_cast<char *>(dataToDecompress.data()), dataToDecompress.size());

    /*
    std::uint16_t blockIndex = 0;
    std::uint16_t previousBlockSize = 0;
    std::uint32_t proceededBytes = 0;
    using Block = std::vector<std::byte>;
    auto remainingBytes = dataToDecompress.size();
    constexpr static std::uint64_t MAX_BLOCK_SIZE = 16'383;
    std::vector<Block> blocks;
    deflate::Deflator deflator;

    while (remainingBytes > 0)
    {
        const auto blockSize = std::min(MAX_BLOCK_SIZE, remainingBytes);

        proceededBytes += blockSize;

        Block block;
        block.insert(block.end(), dataToDecompress.begin() + (blockIndex * previousBlockSize), dataToDecompress.begin() + proceededBytes);
        remainingBytes -= blockSize;
        previousBlockSize = static_cast<std::uint16_t>(blockSize);

        blocks.push_back(deflator(block,true));
        ++blockIndex;
    }



    using Block = std::vector<std::byte>;

    std::uint32_t i = 0;
    std::vector<Block> blocks;


    while ((i + 1) < dataToDecompress.size())
    {
        const auto hiBits = static_cast<std::uint16_t>(dataToDecompress[i]);
        const auto loBits = static_cast<std::uint16_t>(dataToDecompress[i + 1]);

        const auto length = (loBits << 8) | hiBits;
        i += 2;

        Block block;
        block.insert(block.end(), dataToDecompress.begin() + i, dataToDecompress.begin() + i + length);
        std::cout << "Block size: " << length << std::endl;

        blocks.push_back(block);
        i += length;
    }
    std::vector<std::byte> decompressedData;
    deflate::Inflator inflator;
    for (auto& block: blocks)
    {
        auto decompressedBlock = inflator(block);
        decompressedData.insert(decompressedData.end(), decompressedBlock.begin(), decompressedBlock.end());
    }
    */

    std::cout << "Compression...\n";
    parallel::ParallelCompression compression;
    auto compressedData = compression.compress(dataToDecompress);
    std::cout << "Compressed data size: " << compressedData.size() << std::endl;

    std::cout << "Decompression...\n";
    parallel::ParallelDecompression decompression;
    auto decompressedData = decompression.decompress(compressedData);


    /*
    auto compressedData = deflate::LZ77::compress(dataToDecompress);
    deflate::DynamicHuffmanEncoder encoder;
    deflate::DynamicHuffmanDecoder decoder(deflate::BitBuffer{encoder.encodeData(compressedData, true)});
    auto decodedMatches = decoder.decodeData();


    auto it = std::ranges::mismatch(compressedData,decodedMatches,[](const deflate::LZ77::Match& lhs,const deflate::LZ77::Match& rhs){ return lhs.distance == rhs.distance && lhs.length == rhs.length && lhs.literal == rhs.literal;});
    std::cout << "Difference position:" << std::ranges::distance(decodedMatches.begin(),it.in2) << std::endl;
    std::cout << "First:(" << static_cast<std::uint16_t>(it.in1->literal) << " , "<< it.in1->length << " , " << it.in1->distance << ")"<< std::endl;
    std::cout << "Second:(" << static_cast<std::uint16_t>(it.in2->literal) << " , "<< it.in2->length << " , " << it.in2->distance << ")"<< std::endl;

    auto decompressedData = deflate::LZ77::decompress(decodedMatches);
    */

    std::ofstream output("test1.bmp", std::ios::binary);
    output.write((char *) decompressedData.data(), decompressedData.size());


    return 0;
}
