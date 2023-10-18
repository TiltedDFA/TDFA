cmake_minimum_required(VERSION 3.16)
project(TDFA VERSION 0.1.0 LANGUAGES CXX)

set(CXX_STANDARD 20)

set(CXX_STANDARD_REQUIRED true)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER "g++")

file(GLOB_RECURSE MY_SOURCES "src/**.cpp")

set(CACHE CMAKE_VERBOSE_MAKEFILE ON)

if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-std=c++20 -g -gdwarf-5 -ggdb -C -O0 -Wall -Wextra -Wpedantic -save-temps -s -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647)
    else()
        add_compile_options(-std=c++20 -O3 -march=native -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647)
    endif()
endif(WIN32)
if(UNIX)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-std=c++20 -g -gdwarf-5 -ggdb -C -O0 -Wall -Wextra -Wpedantic -save-temps -s -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647 -fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    else()
        add_compile_options(-std=c++20 -O3 -march=native -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647)
    endif()
endif(UNIX)

add_executable(TDFA ${MY_SOURCES})


install(TARGETS TDFA)