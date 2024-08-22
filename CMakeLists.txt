cmake_minimum_required(VERSION 3.28)
project(Deflate)

set(CMAKE_CXX_STANDARD 20)
add_executable(Deflate src/main.cpp
        src/deflate/LZ77.cpp
        src/deflate/LZ77.hpp
        src/deflate/BitBuffer.cpp
        src/deflate/BitBuffer.hpp
        src/deflate/FixedHuffmanEncoder.cpp
        src/deflate/FixedHuffmanEncoder.hpp
        src/deflate/FixedHuffmanDecoder.cpp
        src/deflate/FixedHuffmanDecoder.hpp
        src/deflate/HuffmanTree.cpp
        src/deflate/HuffmanTree.hpp
        src/deflate/DynamicHuffmanEncoder.cpp
        src/deflate/DynamicHuffmanEncoder.hpp)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Deflate PRIVATE -mavx -mavx2 -ffast-math)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(Deflate PRIVATE /arch:AVX2)
endif()


