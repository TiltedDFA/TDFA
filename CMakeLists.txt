cmake_minimum_required(VERSION 3.16)
project(TDFA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(TDFA_BUILD_BENCH "Build benchmark executable" ON)
option(TDFA_STATIC "Link libstdc++/libgcc statically" ON)

file(GLOB_RECURSE TDFA_SOURCES CONFIGURE_DEPENDS "src/*.cpp")
list(FILTER TDFA_SOURCES EXCLUDE REGEX "src/main(_bench)?\\.cpp$")

add_library(tdfa_core OBJECT ${TDFA_SOURCES})
target_include_directories(tdfa_core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_options(tdfa_core PRIVATE
    -fno-exceptions
    -fconstexpr-ops-limit=2147483647
    -fconstexpr-loop-limit=2147483647
    $<$<CONFIG:Debug>:-O0 -g -Wall -Wextra -Wpedantic>
    $<$<CONFIG:Release>:-O3 -march=native -flto -flto-partition=one -funroll-loops -mtune=native>
)

add_executable(TDFA src/main.cpp $<TARGET_OBJECTS:tdfa_core>)

if(TDFA_BUILD_BENCH)
    add_executable(TDFA_bench src/main_bench.cpp $<TARGET_OBJECTS:tdfa_core>)
endif()

foreach(target TDFA TDFA_bench)
    if(TARGET ${target})
        target_link_options(${target} PRIVATE
            $<$<CONFIG:Release>:-flto=jobserver>
        )
        if(TDFA_STATIC)
            target_link_options(${target} PRIVATE -static -static-libgcc -static-libstdc++)
        endif()
    endif()
endforeach()
