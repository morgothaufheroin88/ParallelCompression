function(check_avx2_support)
    include(CheckCXXCompilerFlag)
    # Clang-CL needs the same check for AVX2

       if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
           check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2)

           if(COMPILER_SUPPORTS_AVX2)
               message(STATUS "Compiler supports AVX2 instructions.")
           else()
               message(WARNING "Compiler does not support AVX2 instructions.")
           endif()

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") # clang-cl
            check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2_CLANG)

            if(COMPILER_SUPPORTS_AVX2_CLANG)
                message(STATUS "Clang compiler supports AVX2 instructions.")
            else()
                message(WARNING "Clang compiler does not support AVX2 instructions.")
            endif()

        elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU") # clang native
            check_cxx_compiler_flag("-mavx2" COMPILER_SUPPORTS_AVX2_CLANG)

            if(COMPILER_SUPPORTS_AVX2_CLANG)
                message(STATUS "Clang compiler supports AVX2 instructions.")
            else()
                message(WARNING "Clang compiler does not support AVX2 instructions.")
            endif()

        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
           check_cxx_compiler_flag("/arch:AVX2" COMPILER_SUPPORTS_AVX2_MSVC)

           if(COMPILER_SUPPORTS_AVX2_MSVC)
               message(STATUS "MSVC compiler supports AVX2 instructions.")
           else()
               message(WARNING "MSVC compiler does not support AVX2 instructions.")
           endif()
    else()
        message(WARNING "Unsupported compiler for checking avx2 support.")
    endif()
endfunction()
