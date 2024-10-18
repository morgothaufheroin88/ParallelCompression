cmake_minimum_required(VERSION 3.28)
project(Deflate)

function(set_constexpr_limit target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${target} PRIVATE -fconstexpr-ops-limit=67108864)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") # clang-cl
            target_compile_options(${target} PRIVATE /clang:-fconstexpr-steps=67108864)
        elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU") # clang native
            target_compile_options(${target} PRIVATE -fconstexpr-steps=67108864)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${target} PRIVATE /constexpr:steps=67108864)
    else()
        message(WARNING "Unsupported compiler for setting constexpr evaluation limit.")
    endif()
endfunction()