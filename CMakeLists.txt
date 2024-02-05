cmake_minimum_required(VERSION 3.16)
project(TDFA VERSION 0.1.0 LANGUAGES CXX)

set(CXX_STANDARD 20)

set(CXX_STANDARD_REQUIRED true)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(UNIX)
    set(CMAKE_CXX_COMPILER "g++-13")
endif(UNIX)

if(WIN32)
    set(CMAKE_CXX_COMPILER "g++")
endif(WIN32)

file(GLOB_RECURSE MY_SOURCES "src/*.hpp", "src/*.cpp")

set(CACHE CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS_RELEASE "-std=c++20 -O3 -march=native -funroll-loops -mtune=native")

add_link_options(-flto -flto-partition=one -static)

add_executable(TDFA ${MY_SOURCES})


install(TARGETS TDFA)