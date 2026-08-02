cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
endif()

# Compiler selection belongs to the caller (for example, a CLion toolchain,
# CMake preset, or toolchain file). Replacing CMAKE_CXX_COMPILER after project()
# invalidates CMake's configured compiler state and breaks nested projects.
project(TDFA VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20 CACHE STRING "" FORCE)

set(CXX_STANDARD_REQUIRED ON CACHE BOOL "" FORCE)
set(CMAKE_CX)
set(CMAKE_CXX_STANDARD_REQUIRED ON CACHE BOOL "" FORCE)

set(CACHE CMAKE_VERBOSE_MAKEFILE TRUE CACHE BOOL "" FORCE)

set(TDFA_ENGINE_SOURCES
        src/Debug.cpp
        src/MoveGen.cpp
        src/Position.cpp
        src/Search.cpp
        src/TranspositionTable.cpp
        src/Uci.cpp)

set(TDFA_APPLICATION_SOURCES
        src/main.cpp)


if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS_DEBUG "-ggdb -O0 -Wall -Wextra -Wpedantic -save-temps -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647" CACHE STRING "" FORCE)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic -fconstexpr-steps=2147483647" CACHE STRING "" FORCE)
endif()
#set(CMAKE_CXX_FLAGS_RELEASE "-O3 -static-libgcc -fno-exceptions --static -static-libstdc++ -march=native -flto -flto-partition=one -ffat-lto-objects -funroll-loops -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -g -fno-exceptions -march=native -funroll-loops -mtune=native -fconstexpr-ops-limit=2147483647 -fconstexpr-loop-limit=2147483647" CACHE STRING "" FORCE)
#set(CMAKE_EXE_LINKER_FLAGS "-flto=jobserver" CACHE STRING "" FORCE)

add_executable(TDFA
        ${TDFA_ENGINE_SOURCES}
        ${TDFA_APPLICATION_SOURCES}
        src/Board.hpp)

target_compile_features(TDFA PRIVATE cxx_std_20)


install(TARGETS TDFA)

# Testing is deliberately opt-in. A normal TDFA configure therefore has no
# test-only compile definitions, dependencies, or additional targets.
option(BUILD_TESTING "Build the TDFA verification suite" ON)
include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    # The locally available Clang may intentionally lag the installed MSVC STL
    # by one release. This opt-in is confined to verification builds; it lets
    # that otherwise supported Clang exercise ASan/UBSan without affecting the
    # normal GCC-built engine.
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
            AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
        add_compile_definitions(_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH)
        if(TDFA_ENABLE_SANITIZERS OR TDFA_ENABLE_FUZZING)
            # The bundled Windows sanitizer runtimes are release-STL objects.
            add_compile_definitions(
                    _ITERATOR_DEBUG_LEVEL=0
                    _DISABLE_STL_ANNOTATION)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded" CACHE STRING "" FORCE)
        endif()
    endif()

    # Keep the test dependency reproducible. 6ee0826 is the peeled v3.15.0
    # release tag, rather than a moving branch name.
    set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
    set(CATCH_DEVELOPMENT_BUILD OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
            Catch2
            GIT_REPOSITORY https://github.com/catchorg/Catch2.git
            GIT_TAG 6ee0826dcae55ed1e06b2c5701981221e979e1e6
            GIT_PROGRESS TRUE)
    FetchContent_MakeAvailable(Catch2)

    option(TDFA_ENABLE_COVERAGE "Instrument the verification suite with GCC/gcov" OFF)
    option(TDFA_ENABLE_SANITIZERS "Instrument the verification suite with ASan and UBSan" OFF)
    option(TDFA_ENABLE_FUZZING "Build libFuzzer verification targets" OFF)

    if(TDFA_ENABLE_COVERAGE AND (TDFA_ENABLE_SANITIZERS OR TDFA_ENABLE_FUZZING))
        message(FATAL_ERROR "Coverage, sanitizer, and fuzz instrumentation use separate build trees")
    endif()

    # This is the exact production implementation, only without the application
    # entry point. TDFA_TESTING may expose tightly reviewed friend probes, but it
    # must never select alternate runtime behavior.
    add_library(tdfa_test_subject STATIC ${TDFA_ENGINE_SOURCES})
    target_compile_features(tdfa_test_subject PUBLIC cxx_std_20)
    target_compile_definitions(tdfa_test_subject PUBLIC TDFA_TESTING=1)
    target_include_directories(tdfa_test_subject PUBLIC "${PROJECT_SOURCE_DIR}/src")

    if(TDFA_ENABLE_COVERAGE)
        if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
            message(FATAL_ERROR "TDFA_ENABLE_COVERAGE requires GCC/gcov")
        endif()

        add_library(tdfa_coverage_options INTERFACE)
        target_compile_options(tdfa_coverage_options INTERFACE
                --coverage
                -O0
                -g
                -fprofile-abs-path
                -fcondition-coverage)
        target_link_options(tdfa_coverage_options INTERFACE --coverage)
        target_link_libraries(tdfa_test_subject PUBLIC tdfa_coverage_options)
        target_link_libraries(TDFA PRIVATE tdfa_coverage_options)
    endif()

    if(TDFA_ENABLE_SANITIZERS)
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
            message(FATAL_ERROR "TDFA_ENABLE_SANITIZERS requires Clang or GCC")
        endif()

        add_library(tdfa_sanitizer_options INTERFACE)
        target_compile_options(tdfa_sanitizer_options INTERFACE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
                -fno-sanitize-recover=all)
        if(CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
            # Clang 18's Windows ASan otherwise emits associative globals that
            # the newer MSVC STL cannot assemble. The forced standard header
            # also avoids relying on GCC-only transitive <bit> inclusion.
            target_compile_options(tdfa_sanitizer_options INTERFACE
                    -include bit
                    -mllvm -asan-globals=0)
        endif()
        target_link_options(tdfa_sanitizer_options INTERFACE
                -fsanitize=address,undefined
                -fno-sanitize-recover=all)
        target_link_libraries(tdfa_test_subject PUBLIC tdfa_sanitizer_options)
        target_link_libraries(TDFA PRIVATE tdfa_sanitizer_options)
    endif()

    if(TDFA_ENABLE_FUZZING)
        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR "TDFA_ENABLE_FUZZING requires Clang with libFuzzer support")
        endif()

        add_library(tdfa_fuzz_subject_options INTERFACE)
        target_compile_options(tdfa_fuzz_subject_options INTERFACE
                -fsanitize=fuzzer-no-link,address,undefined
                -fno-omit-frame-pointer
                -fno-sanitize-recover=all)
        if(CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
            target_compile_options(tdfa_fuzz_subject_options INTERFACE
                    -include bit
                    -mllvm -asan-globals=0)
        endif()
        target_link_libraries(tdfa_test_subject PUBLIC tdfa_fuzz_subject_options)
    endif()

    add_subdirectory(tests)
endif()
