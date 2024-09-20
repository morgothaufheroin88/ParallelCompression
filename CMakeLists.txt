cmake_minimum_required(VERSION 3.28)
project(Deflate)

set(CMAKE_CXX_STANDARD 20)
add_executable(Deflate src/main.cpp
        src/deflate/lz/LZ77.cpp
        src/deflate/lz/LZ77.hpp
        src/deflate/buffer/BitBuffer.cpp
        src/deflate/buffer/BitBuffer.hpp
        src/deflate/encoders/FixedHuffmanEncoder.cpp
        src/deflate/encoders/FixedHuffmanEncoder.hpp
        src/deflate/decoders/FixedHuffmanDecoder.cpp
        src/deflate/decoders/FixedHuffmanDecoder.hpp
        src/deflate/tree/HuffmanTree.cpp
        src/deflate/tree/HuffmanTree.hpp
        src/deflate/encoders/DynamicHuffmanEncoder.cpp
        src/deflate/encoders/DynamicHuffmanEncoder.hpp
        src/deflate/decoders/DynamicHuffmanDecoder.cpp
        src/deflate/decoders/DynamicHuffmanDecoder.hpp)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Deflate PRIVATE -mavx -mavx2 -ffast-math)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(Deflate PRIVATE /arch:AVX2)
endif()


