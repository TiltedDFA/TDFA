cmake_minimum_required(VERSION 3.16)
project(TDFA VERSION 0.1.0 LANGUAGES CXX)

set_property(TARGET CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_COMPILER "g++")

file(GLOB_RECURSE MY_SOURCES "src/**.cpp")

if(CMAKE_CXX_FLAGS_DEBUG)
    add_compile_options(-g -O0 -Wall -Wextra -Wpedantic -save-temps)
else()
    add_compile_options(-O3 -march=native -save-temps)
endif()


add_executable(TDFA ${MY_SOURCES})


install(TARGETS TDFA)