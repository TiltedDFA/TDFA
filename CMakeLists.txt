cmake_minimum_required(VERSION 3.16)
project(TDFA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "" FORCE)

set(CXX_STANDARD_REQUIRED ON CACHE BOOL "" FORCE)

set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "" FORCE)

set(CACHE CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOL "" FORCE)

if(UNIX)
    set(CMAKE_CXX_COMPILER "g++-13" CACHE STRING "" FORCE)
endif(UNIX)

if(WIN32)
    set(CMAKE_CXX_COMPILER "g++" CACHE STRING "" FORCE)
endif(WIN32)

file(GLOB_RECURSE MY_SOURCES "src/*.cpp")


set(CMAKE_CXX_FLAGS_DEBUG "-g3 -gdwarf-5 -ggdb -C -O0 -Wall -Wextra -Wpedantic -save-temps -s -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -static-libgcc -fno-exceptions --static -static-libstdc++ -march=native -flto -flto-partition=one -ffat-lto-objects -funroll-loops -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS "-flto=jobserver -static" CACHE STRING "" FORCE)

add_executable(TDFA ${MY_SOURCES})


install(TARGETS TDFA)