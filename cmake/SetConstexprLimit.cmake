cmake_minimum_required(VERSION 3.12)

function(set_constexpr_limit target)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_compile_options(${target} PUBLIC -fconstexpr-ops-limit=67108864)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        if (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC") # clang-cl
            target_compile_options(${target} PUBLIC /clang:-fconstexpr-steps=67108864)
        elseif (CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "GNU") # clang native
            target_compile_options(${target} PUBLIC -fconstexpr-steps=67108864)
        endif()
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_options(${target} PUBLIC /constexpr:steps67108864)
    else()
        message(WARNING "Unsupported compiler for setting constexpr evaluation limit.")
    endif()
endfunction()