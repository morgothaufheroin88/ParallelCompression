cmake_minimum_required(VERSION 3.28)
project(Deflate)
set(CMAKE_CXX_STANDARD 20)

set(TARGET_NAME ParallelCompression)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")


include(ConfigurePGO)
include(SetConstexprLimit)
include(CheckCompilerFlags)

add_executable(${TARGET_NAME} src/main.cpp
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
        src/compression/deflate/Inflator.hpp
        src/compression/parallel/ParallelCompression.cpp
        src/compression/parallel/ParallelCompression.hpp
        src/compression/parallel/ParallelDecompression.cpp
        src/compression/parallel/ParallelDecompression.hpp)



configure_pgo(${TARGET_NAME})
set_constexpr_limit(${TARGET_NAME})
check_avx2_support(${TARGET_NAME})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Проверка на тип компилятора и установка флагов
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_CXX_FLAGS_RELEASE "-O2 -mavx2")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(CMAKE_CXX_FLAGS_RELEASE "/O2 /arch:AVX2")
    endif()
endif()