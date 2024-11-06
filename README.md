# ParallelCompression

**ParallelCompression** is a C++ library implementing compression and decompression based on the [RFC 1951](https://www.ietf.org/rfc/rfc1951.txt) standard (the DEFLATE algorithm). Data is split into blocks and compressed in parallel to improve performance. Each compressed block in the output file includes the compressed data, a CRC32 checksum, and the block size.

## Key Features

- **Parallel Compression Support**: Data is split into blocks, with each block compressed in a separate thread.
- **RFC 1951 Compliance**: Compression follows the DEFLATE standard.
- **CRC32 Checksums**: CRC32 checksum is calculated for each block to verify data integrity.
- **Metadata Recording**: Each compressed block includes its size and CRC32 checksum.

## Requirements

- **Compiler**: Supported compilers include GCC, Clang, and MSVC.
- **C++ Standard**: C++20
- **CMake**: version 3.12 or later

## Installation and Build

1. Clone the repository:

    ```bash
    git clone https://github.com/l09dm0zgus/ParallelCompression.git
    cd ParallelCompression
    ```

2. Create a build directory and navigate into it:

    ```bash
    mkdir build
    cd build
    ```

3. Run CMake and build the library:

    ```bash
    cmake ..
    cmake --build .
    ```

After building, the library file will be available in the `build` directory.

## Using the Library

To use **ParallelCompression** in your project, include the library and link it in your CMake configuration:

1. Add the **ParallelCompression** library as a subdirectory in your project:

    ```cmake
    add_subdirectory(path/to/ParallelCompression)
    ```

2. Link **ParallelCompression** to your target:

    For static linking:

    ```cmake
    target_link_libraries(YourTarget PRIVATE ParallelCompression::Static)
    ``` 
   For dynamic linking:

    ```cmake
    target_link_libraries(YourTarget PRIVATE ParallelCompression::Shared)
    ```

3. Include the header files from **ParallelCompression** to access its functionality for compressing and decompressing data blocks.

### Example Code

Here's an example of how to use **ParallelCompression** to compress and decompress files:
```cpp 
   #include "compression/Compression.hpp"

    // Example usage
    std::cout << "Compression...\n";
    parallel::ParallelCompression compression;
    
    /* Compression Levels
     * Level 1 - fastest compression, lowest compression ratio.
     * Level 6 - slowest compression, bigger compression ratio.
     */
     
    auto compressedData = compression.compress(dataToDecompress,deflate::CompressionLevel::LEVEL_3);
    std::cout << "Compressed data size: " << compressedData.size() << std::endl;

    std::cout << "Decompression...\n";
    parallel::ParallelDecompression decompression;
    auto decompressedData = decompression.decompress(compressedData);
```