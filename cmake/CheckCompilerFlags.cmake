function(check_sse42_support)
    include(CheckCXXCompilerFlag)
    # Clang-CL needs the same check for AVX2

       if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
           set(CMAKE_CXX_FLAGS_RELEASE "-O2")

           check_cxx_compiler_flag("-msse4.2" SSE4_2_SUPPORTED)
           if(SSE4_2_SUPPORTED)
               message(STATUS "Compiler supports SSE4.2")
               add_compile_definitions(SSE42_SUPPORTED)
               set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
           else()
               message(STATUS "Compiler does not support SSE4.2")
           endif()

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") # clang-cl
            set(CMAKE_CXX_FLAGS_RELEASE "/O2")

            check_cxx_compiler_flag("/arch:SSE4.2" SSE4_2_SUPPORTED)
            if(SSE4_2_SUPPORTED)
                message(STATUS "Compiler supports SSE4.2")
                add_compile_definitions(SSE42_SUPPORTED)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.2")
            else()
                message(STATUS "Compiler does not support SSE4.2")
            endif()

        elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU") # clang native
            set(CMAKE_CXX_FLAGS_RELEASE "-O2")

            check_cxx_compiler_flag("-msse4.2" SSE4_2_SUPPORTED)
            if(SSE4_2_SUPPORTED)
                message(STATUS "Compiler supports SSE4.2")
                add_compile_definitions(SSE42_SUPPORTED)
                set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse4.2")
            else()
                message(STATUS "Compiler does not support SSE4.2")
            endif()

        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
           set(CMAKE_CXX_FLAGS_RELEASE "/O2")

           check_cxx_compiler_flag("/arch:SSE4.2" SSE4_2_SUPPORTED)
           if(SSE4_2_SUPPORTED)
               message(STATUS "Compiler supports SSE4.2")
               add_compile_definitions(SSE42_SUPPORTED)
               set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE4.2")
           else()
               message(STATUS "Compiler does not support SSE4.2")
           endif()
    else()
        message(WARNING "Unsupported compiler.")
       endif()
endfunction()
