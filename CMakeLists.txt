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

# -static-libgcc --static -static-libstdc++
if(WIN32)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    #    add_compile_options(-std=c++20 -g -gdwarf-5 -ggdb -C -O0 -Wall -Wextra -Wpedantic -save-temps -s -fanalyzer -Wconversion -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647)
        add_compile_options(-std=c++20 -g3 -gdwarf-5 -ggdb -C -O0 -Wall -Wconversion -Wextra -Wpedantic -save-temps -s -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647)
    else()
        # add_compile_options(-std=c++20 -g -gdwarf-5 -ggdb -C -O0 -Wall -Wextra -Wpedantic -save-temps -s -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647)
        # add_compile_options(-std=c++20 -O3 -march=native -funroll-loops -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647)
        # add_compile_options(-std=c++20 -O3 -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647 -static-libgcc -static-libstdc++ -funroll-loops)
        set(CMAKE_CXX_FLAGS_RELEASE "-std=c++20 -O3 -static-libgcc --static -static-libstdc++ -march=native -funroll-loops -mtune=native")
        # set(CMAKE_CXX_FLAGS_RELEASE "-std=c++20 -Oz -static-libgcc --static -static-libstdc++ -march=native -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647")
        add_link_options(-flto -flto-partition=one -static)
    endif()
endif(WIN32)
if(UNIX)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-std=c++20 -g3 -gdwarf-5 -ggdb -C -O0 -Wall -Wextra -pedantic-errors -Wpedantic -save-temps -s -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647 -fsanitize=undefined)
        add_link_options(-fsanitize=undefined)
    else()
        # with debug info
        add_compile_options(-std=c++20 -O3 -march=native -g3 -ggdb -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647 -funroll-loops)

        #without debug info
        # add_compile_options(-std=c++20 -O3 -march=native -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647 -funroll-loops)
    endif()
endif(UNIX)

add_executable(TDFA ${MY_SOURCES})


install(TARGETS TDFA)