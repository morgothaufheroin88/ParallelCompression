cmake_minimum_required(VERSION 3.28)
project(Deflate)

set(CMAKE_CXX_STANDARD 20)
add_executable(Deflate src/main.cpp
        src/compression/lz/LZ77.cpp
        src/compression/lz/LZ77.hpp
        src/compression/buffer/BitBuffer.cpp
        src/compression/buffer/BitBuffer.hpp
        src/compression/encoders/FixedHuffmanEncoder.cpp
        src/compression/encoders/FixedHuffmanEncoder.hpp
        src/compression/decoders/FixedHuffmanDecoder.cpp
        src/compression/decoders/FixedHuffmanDecoder.hpp
        src/compression/tree/HuffmanTree.cpp
        src/compression/tree/HuffmanTree.hpp
        src/compression/encoders/DynamicHuffmanEncoder.cpp
        src/compression/encoders/DynamicHuffmanEncoder.hpp
        src/compression/decoders/DynamicHuffmanDecoder.cpp
        src/compression/decoders/DynamicHuffmanDecoder.hpp
        src/compression/Compression.hpp
        src/compression/deflate/Deflator.cpp
        src/compression/deflate/Deflator.hpp
        src/compression/deflate/Inflator.cpp
        src/compression/deflate/Inflator.hpp)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Deflate PRIVATE -mavx -mavx2 -ffast-math)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(Deflate PRIVATE /arch:AVX2)
endif()


