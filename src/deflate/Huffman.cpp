//
// Created by cx9ps3 on 01.08.2024.
//

#include "Huffman.hpp"
#include <algorithm>
#include <bitset>
#include <iostream>
#include <ranges>
#include <stack>


deflate::Huffman::TreeNodes deflate::Huffman::buildLiteralsAndLengthsTree(const std::vector<std::uint32_t> &literalsFrequencies, const std::vector<std::uint32_t> &lengthsFrequencies)
{
    MinimalHeap minimalHeap;
    TreeNodes treeNodes;
    treeNodes.reserve(literalsFrequencies.size() + lengthsFrequencies.size());

    for (std::int16_t i = 0; i < static_cast<std::int16_t>(literalsFrequencies.size()); i++)
    {
        if (literalsFrequencies[i] > 0)
        {
            Node node;
            node.frequency = literalsFrequencies[i];
            node.symbol = i;
            node.nodeType = NodeType::LITERAL;
            treeNodes.push_back(node);
            minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));
        }
    }

    for (std::int16_t i = 0; i < static_cast<std::int16_t>(lengthsFrequencies.size()); i++)
    {
        if (lengthsFrequencies[i])
        {
            Node node;
            node.frequency = lengthsFrequencies[i];
            node.symbol = MAX_LITERAL + i;
            node.nodeType = NodeType::LENGTH;
            treeNodes.push_back(node);
            minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));
        }
    }

    buildTree(minimalHeap, treeNodes);
    calculateCodesLengths(treeNodes, static_cast<std::uint32_t>(treeNodes.size() - 1));
    return treeNodes;
}

void deflate::Huffman::calculateCodesLengths(deflate::Huffman::TreeNodes &treeNodes, std::uint32_t rootIndex)
{
    if (rootIndex == -1)
    {
        return;
    }

    std::stack<std::pair<std::uint32_t, std::uint8_t>> stack;
    stack.emplace(rootIndex, 0);

    while (!stack.empty())
    {
        auto [nodesIndex, currentLength] = stack.top();
        stack.pop();

        auto &node = treeNodes[nodesIndex];
        if ((node.frequency != 0) && (node.leftChildId == -1) && (node.rightChildId == -1))
        {
            node.codeLength = currentLength;
        }
        else
        {
            if (node.rightChildId != -1)
            {
                stack.emplace(node.rightChildId, currentLength + 1);
            }
            if (node.leftChildId != -1)
            {
                stack.emplace(node.leftChildId, currentLength + 1);
            }
        }
    }
}

std::vector<std::byte> deflate::Huffman::encodeWithDynamicCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock)
{
    std::vector<std::byte> compressedData;
    compressedData.reserve(lz77CompressedData.size());

    const auto [literalsFrequencies, lengthsFrequencies, distancesFrequencies] = countFrequencies(lz77CompressedData);

    auto literalsCodeTable = createCodeTableForLiterals(literalsFrequencies, lengthsFrequencies);

    auto distancesCodeTable = createCodeTableForDistances(distancesFrequencies);

    return compressedData;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize)
{
    DynamicCodeTable codeTable(codeTableSize);
    codeTable.reserve(codeLengths.size());

    std::array<std::uint16_t, MAX_BITS + 1> codeLengthsCount = {0};
    std::array<std::uint16_t, MAX_BITS + 1> nextCode = {0};

    for (const auto &length: codeLengths)
    {
        codeLengthsCount[length]++;
    }

    std::uint16_t code{0};
    for (std::uint8_t bit = 1; bit <= MAX_BITS; bit++)
    {
        code = static_cast<std::uint16_t>((code + codeLengthsCount[bit - 1]) << 1);
        nextCode[bit] = code;
    }

    std::uint16_t symbol{0};
    for (auto length: codeLengths)
    {
        if (length != 0)
        {
            std::cout << symbol << " " << std::bitset<16>(nextCode[length]).to_string().substr(16 - length, 16) << "\n";

            CanonicalHuffmanCode canonicalHuffmanCode;
            canonicalHuffmanCode.code = nextCode[length];
            canonicalHuffmanCode.length = length;
            codeTable[symbol] = canonicalHuffmanCode;
            nextCode[length]++;
        }
        symbol++;
    }

    return codeTable;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createReverseCodeTable(const std::vector<std::uint8_t> &codeLengths, std::uint16_t codeTableSize)
{
    auto codeTable = createCodeTable(codeLengths, codeTableSize);
    DynamicCodeTable reverseCodeTable;
    reverseCodeTable.reserve(codeLengths.size());

    for (const auto &[code, symbol]: codeTable)
    {
        reverseCodeTable[code] = symbol;
    }
    return reverseCodeTable;
}

std::vector<std::byte> deflate::Huffman::encodeWithFixedCodes(const std::vector<LZ77::Match> &lz77CompressedData, bool isLastBlock)
{
    std::vector<std::byte> compressedData;
    compressedData.reserve(lz77CompressedData.size());

    std::byte currentByte{0};
    std::uint8_t bitPosition = 0;

    //write header
    if (isLastBlock)
    {
        addBitsToBuffer(compressedData, 1, 1, bitPosition, currentByte);
    }
    else
    {
        addBitsToBuffer(compressedData, 0, 1, bitPosition, currentByte);
    }

    addBitsToBuffer(compressedData, 0b01, 2, bitPosition, currentByte);


    for (const auto &lz77Match: lz77CompressedData)
    {
        if (lz77Match.length > 1)
        {
            const auto &lengthCode = FIXED_LENGTHS_CODES[lz77Match.length];
            const auto &distanceCode = FIXED_DISTANCES_CODES[lz77Match.distance];

            //encode lz77 lengths
            addBitsToBuffer(compressedData, lengthCode.code, lengthCode.codeLength, bitPosition, currentByte);
            addBitsToBuffer(compressedData, lengthCode.extraBits, lengthCode.extraBitsCount, bitPosition, currentByte);

            //encode lz77 distance
            addBitsToBuffer(compressedData, distanceCode.code, 5, bitPosition, currentByte);
            addBitsToBuffer(compressedData, distanceCode.extraBits, distanceCode.extraBitsCount, bitPosition, currentByte);
        }
        else
        {
            const auto &literalCode = FIXED_LITERALS_CODES[static_cast<std::uint8_t>(lz77Match.literal)];

            //encode lz77 literal
            addBitsToBuffer(compressedData, literalCode.code, literalCode.codeLength, bitPosition, currentByte);
        }
    }

    //write end of block
    addBitsToBuffer(compressedData, 0, 7, bitPosition, currentByte);
    if (bitPosition > 0)
    {
        compressedData.push_back(currentByte);
    }

    return compressedData;
}

void deflate::Huffman::addBitsToBuffer(std::vector<std::byte> &buffer, std::uint16_t value, std::uint8_t bitCount, uint8_t &bitPosition, std::byte &currentByte)
{
    while (bitCount > 0)
    {
        auto bitsToWrite = std::min(bitCount, static_cast<std::uint8_t>(8 - bitPosition));
        auto bits = std::byte((value & ((1 << bitsToWrite) - 1)) << bitPosition);
        currentByte |= bits;

        bitPosition += bitsToWrite;
        bitCount -= bitsToWrite;
        value >>= bitsToWrite;

        if (bitPosition == 8)
        {
            buffer.push_back(currentByte);
            currentByte = std::byte{0};
            bitPosition = 0;
        }
    }
}

std::vector<deflate::LZ77::Match> deflate::Huffman::decodeWithFixedCodes(const std::vector<std::byte> &compressedData)
{

    std::vector<LZ77::Match> lz77Decompressed;
    lz77Decompressed.reserve(compressedData.size());

    std::uint8_t byteBitPosition{0};
    std::uint8_t codeBitPosition{0};
    std::size_t byteIndex{0};
    std::uint16_t code{0};
    std::byte currentLiteral{0};
    std::uint16_t currentLength{0};
    std::uint16_t currentDistance{0};
    std::uint8_t extraBits = 0;
    bool isNextDistance = false;

    bool isHeaderParsed = false;
    auto currentByte = compressedData[0];

    auto readBit = [&]()
    {
        auto bit = (static_cast<std::uint16_t>(currentByte) >> byteBitPosition) & 1;
        byteBitPosition++;
        if (byteBitPosition == 8)
        {
            byteIndex++;
            byteBitPosition = 0;
            currentByte = compressedData[byteIndex];
        }
        return bit;
    };

    auto findCode = [&code](const auto &arrayCode)
    {
        return arrayCode.code == code;
    };

    auto findByExtraBits = [&extraBits, &code](const auto &distanceCode)
    {
        return extraBits == distanceCode.extraBits && code == distanceCode.code;
    };

    auto tryDecodeLength = [&]()
    {
        //find length with code
        auto lengthCodesIterator = std::ranges::find_if(FIXED_LENGTHS_CODES, findCode);
        if (lengthCodesIterator != FIXED_LENGTHS_CODES.cend())
        {
            //try read extra bits if code has extra bits
            isNextDistance = true;
            if (lengthCodesIterator->extraBitsCount > 0)
            {
                for (std::uint8_t i = 0; i < lengthCodesIterator->extraBitsCount; i++)
                {
                    extraBits |= (readBit() << i);
                }

                lengthCodesIterator = std::ranges::find_if(FIXED_LENGTHS_CODES, findByExtraBits);

                if (lengthCodesIterator != FIXED_LENGTHS_CODES.cend())
                {
                    currentLength = lengthCodesIterator->length;
                }
            }
            else
            {
                currentLength = lengthCodesIterator->length;
            }

            //reset for new code
            code = 0;
            codeBitPosition = 0;
            extraBits = 0;
        }
    };

    auto tryDecodeDistance = [&]()
    {
        //find distance with code
        if (auto distanceCodeIterator = std::ranges::find_if(FIXED_DISTANCES_CODES, findCode); distanceCodeIterator != FIXED_DISTANCES_CODES.cend())
        {
            //try read extra bits if code has extra bits
            if (distanceCodeIterator->extraBitsCount > 0)
            {
                for (std::uint8_t i = 0; i < distanceCodeIterator->extraBitsCount; i++)
                {
                    extraBits |= (readBit() << i);
                }

                distanceCodeIterator = std::ranges::find_if(FIXED_DISTANCES_CODES, findByExtraBits);

                if (distanceCodeIterator != FIXED_DISTANCES_CODES.cend())
                {
                    currentDistance = distanceCodeIterator->distance;
                }
            }
            else
            {
                currentDistance = distanceCodeIterator->distance;
            }
        }

        //reset for new code
        code = 0;
        codeBitPosition = 0;
        extraBits = 0;
    };

    while (byteIndex < compressedData.size())
    {

        //read one bit from byte
        code |= (readBit() << codeBitPosition);
        codeBitPosition++;

        //check header magic numbers
        if (!isHeaderParsed && (codeBitPosition == 3) && ((code == 0b011) || (code == 0b010)))
        {
            isHeaderParsed = true;
            codeBitPosition = 0;
            code = 0;
        }

        //check if code is literal or encoded lz77 length with code length = 8
        //code length with 8 - literals from 0 to 143 or lz77 length from 280 to 287
        //code length with 9 - literals from 144 to 255
        //see - https://datatracker.ietf.org/doc/html/rfc1951#page-12 for details

        if ((codeBitPosition == 8) || (codeBitPosition == 9))
        {
            if (auto literalsCodesIterator = std::ranges::find_if(FIXED_LITERALS_CODES, findCode); literalsCodesIterator != FIXED_LITERALS_CODES.cend())
            {
                currentLiteral = literalsCodesIterator->literal;
                code = 0;
                codeBitPosition = 0;
            }
            else
            {
                tryDecodeLength();
            }
            if (codeBitPosition == 9)
            {
                code = 0;
                codeBitPosition = 0;
            }
        }
        //check if code is  lz77 length with code length = 7
        //see - https://datatracker.ietf.org/doc/html/rfc1951#page-12 for details
        else if (codeBitPosition == 7)
        {
            tryDecodeLength();
        }
        //check if code is  lz77 distance with code length = 5
        //see - https://datatracker.ietf.org/doc/html/rfc1951#page-12 for details
        else if (codeBitPosition == 5 && isNextDistance)
        {
            isNextDistance = false;
            tryDecodeDistance();
        }

        //adding current decoded literal
        if (static_cast<std::uint8_t>(currentLiteral) != 0)
        {
            lz77Decompressed.emplace_back(currentLiteral, 0, 1);
            currentLiteral = std::byte{0};
        }
        //adding current decoded length and distance
        else if ((currentDistance != 0) && (currentLength != 0))
        {
            lz77Decompressed.emplace_back(std::byte{0}, currentDistance, currentLength);
            currentDistance = 0;
            currentLength = 0;
        }
    }
    return lz77Decompressed;
}

deflate::Huffman::Frequencies deflate::Huffman::countFrequencies(const std::vector<LZ77::Match> &lz77Matches)
{
    std::vector<std::uint32_t> literalsFrequencies(MAX_LITERAL, 0);
    std::vector<uint32_t> lengthsFrequencies(LITERALS_AND_DISTANCES_ALPHABET_SIZE - MAX_LITERAL, 0);
    std::vector<uint32_t> distancesFrequencies(DISTANCES_ALPHABET_SIZE, 0);

    for (const auto &match: lz77Matches)
    {
        if (match.length > 1)
        {
            const auto &lengthCode = FIXED_LENGTHS_CODES[match.length];
            const auto &distanceCode = FIXED_DISTANCES_CODES[match.distance];

            lengthsFrequencies[lengthCode.index]++;
            distancesFrequencies[distanceCode.index]++;
        }
        else
        {
            literalsFrequencies[static_cast<std::uint8_t>(match.literal)]++;
        }
    }

    Frequencies frequencies;
    std::get<0>(frequencies) = literalsFrequencies;
    std::get<1>(frequencies) = lengthsFrequencies;
    std::get<2>(frequencies) = distancesFrequencies;

    return frequencies;
}

void deflate::Huffman::buildTree(std::priority_queue<std::uint32_t, std::vector<std::uint32_t>, NodeCompare> &minimalHeap, TreeNodes &treeNodes)
{
    while (minimalHeap.size() > 1)
    {
        auto left = minimalHeap.top();
        minimalHeap.pop();

        auto right = minimalHeap.top();
        minimalHeap.pop();

        auto newFrequency = treeNodes[left].frequency + treeNodes[right].frequency;
        Node node;
        node.frequency = newFrequency;
        node.leftChildId = static_cast<std::int32_t>(left);
        node.rightChildId = static_cast<std::int32_t>(right);
        treeNodes.push_back(node);

        auto parentIndex = treeNodes.size() - 1;
        treeNodes[left].parentId = static_cast<std::uint32_t>(parentIndex);
        treeNodes[right].parentId = static_cast<std::uint32_t>(parentIndex);

        minimalHeap.push(static_cast<std::int32_t>(parentIndex));
    }
}

std::vector<std::uint8_t> deflate::Huffman::getLengthsFromNodes(const deflate::Huffman::TreeNodes &treeNodes, std::uint16_t size)
{
    std::vector<std::uint8_t> lengths(size, 0);
    for (const auto &node: treeNodes)
    {
        if (node.symbol > -1)
        {
            lengths[node.symbol] = node.codeLength;
        }
    }
    return lengths;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createCodeTableForLiterals(const std::vector<uint32_t> &literalsFrequencies, const std::vector<uint32_t> &lengthsFrequencies)
{
    auto literalsAndLengthsTreeNodes = buildLiteralsAndLengthsTree(literalsFrequencies, lengthsFrequencies);

    if (literalsAndLengthsTreeNodes.size() == 1)
    {
        DynamicCodeTable codeTable(LITERALS_AND_DISTANCES_ALPHABET_SIZE);
        CanonicalHuffmanCode code;
        code.code = 1;
        code.length = 1;
        codeTable[literalsAndLengthsTreeNodes[0].symbol] = code;
        return codeTable;
    }

    std::ranges::sort(literalsAndLengthsTreeNodes, NodeSortCompare());
    auto literalsCodesLengths = getLengthsFromNodes(literalsAndLengthsTreeNodes, LITERALS_AND_DISTANCES_ALPHABET_SIZE + 1);

    return createCodeTable(literalsCodesLengths, LITERALS_AND_DISTANCES_ALPHABET_SIZE);
}

deflate::Huffman::TreeNodes deflate::Huffman::buildDistancesTree(const std::vector<std::uint32_t> &distancesFrequencies)
{
    MinimalHeap minimalHeap;
    TreeNodes treeNodes;
    treeNodes.reserve(distancesFrequencies.size());

    for (std::int16_t i = 0; i < static_cast<std::int16_t>(distancesFrequencies.size()); i++)
    {
        if (distancesFrequencies[i] > 0)
        {
            Node node;
            node.frequency = distancesFrequencies[i];
            node.symbol = i;
            node.nodeType = NodeType::DISTANCE;
            treeNodes.push_back(node);
            minimalHeap.push(static_cast<std::uint32_t>(treeNodes.size() - 1));
        }
    }

    buildTree(minimalHeap, treeNodes);
    calculateCodesLengths(treeNodes, static_cast<std::uint32_t>(treeNodes.size() - 1));
    return treeNodes;
}

deflate::Huffman::DynamicCodeTable deflate::Huffman::createCodeTableForDistances(const std::vector<uint32_t> &distancesFrequencies)
{
    auto distancesTreeNodes = buildDistancesTree(distancesFrequencies);

    if (distancesTreeNodes.size() == 1)
    {
        DynamicCodeTable codeTable(DISTANCES_ALPHABET_SIZE);
        CanonicalHuffmanCode code;
        code.code = 1;
        code.length = 1;
        codeTable[distancesTreeNodes[0].symbol] = code;
        return codeTable;
    }

    std::ranges::sort(distancesTreeNodes, NodeSortCompare());
    auto literalsCodesLengths = getLengthsFromNodes(distancesTreeNodes, DISTANCES_ALPHABET_SIZE + 1);

    return createCodeTable(literalsCodesLengths, DISTANCES_ALPHABET_SIZE);
}
