include("${catch2_SOURCE_DIR}/extras/Catch.cmake")

set(TDFA_TEST_RUNTIME_PATHS "C:/msys64/ucrt64/bin" CACHE STRING
        "Semicolon-separated runtime-library directories for TDFA tests")
set(TDFA_CLANG_SANITIZER_RUNTIME
        "C:/Users/Student/Documents/clang+llvm-18.1.8-x86_64-pc-windows-msvc/lib/clang/18/lib/windows"
        CACHE PATH "Clang sanitizer runtime-library directory")
set(TDFA_FAST_TIMEOUT 30)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang"
        AND CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC"
        AND TDFA_CLANG_SANITIZER_RUNTIME)
    list(APPEND TDFA_TEST_RUNTIME_PATHS "${TDFA_CLANG_SANITIZER_RUNTIME}")
endif()
if(TDFA_ENABLE_SANITIZERS)
    set(TDFA_FAST_TIMEOUT 180)
endif()

# Test files are intentionally discovered here so independently authored test
# packets can land without repeatedly editing build metadata. Production source
# files remain explicitly enumerated in the root CMakeLists.txt.
file(GLOB_RECURSE TDFA_TEST_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
list(FILTER TDFA_TEST_SOURCES EXCLUDE REGEX "[/\\\\]fuzz[/\\\\]")

if(NOT TDFA_TEST_SOURCES)
    message(FATAL_ERROR
            "BUILD_TESTING is ON, but tests/ contains no C++ test sources. "
            "A verification build must never succeed with zero tests.")
endif()

add_executable(tdfa_tests ${TDFA_TEST_SOURCES})
target_compile_features(tdfa_tests PRIVATE cxx_std_20)
target_compile_definitions(tdfa_tests PRIVATE
        TDFA_TESTING=1
        TDFA_ENGINE_PATH="$<TARGET_FILE:TDFA>")
target_include_directories(tdfa_tests PRIVATE "${PROJECT_SOURCE_DIR}/tests")
target_link_libraries(tdfa_tests PRIVATE tdfa_test_subject Catch2::Catch2WithMain)
if(WIN32)
    target_link_libraries(tdfa_tests PRIVATE bcrypt)
endif()
add_dependencies(tdfa_tests TDFA)

# A test belongs to exactly one primary tier. Untagged and ordinary tagged tests
# are fast; expensive suites opt into [deep] or [adversarial]. Prefixes keep
# repeated Catch discovery invocations unambiguous in CTest.
catch_discover_tests(tdfa_tests
        TEST_PREFIX "fast::"
        TEST_SPEC "~[deep]~[adversarial]~[mutation]~[audit]~[uci]"
        DL_PATHS ${TDFA_TEST_RUNTIME_PATHS}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTIES LABELS "fast" TIMEOUT ${TDFA_FAST_TIMEOUT}
        DISCOVERY_MODE PRE_TEST)

# Process tests are serialized so independent engine children cannot race on
# coverage files, inherited console state, or hard-cleanup resources.
catch_discover_tests(tdfa_tests
        TEST_PREFIX "uci::"
        TEST_SPEC "[uci]~[deep]~[adversarial]~[mutation]~[audit]"
        DL_PATHS ${TDFA_TEST_RUNTIME_PATHS}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTIES LABELS "fast" TIMEOUT 90 RUN_SERIAL TRUE
        DISCOVERY_MODE PRE_TEST)

catch_discover_tests(tdfa_tests
        TEST_PREFIX "deep::"
        TEST_SPEC "[deep]~[audit]"
        DL_PATHS ${TDFA_TEST_RUNTIME_PATHS}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTIES LABELS "deep" TIMEOUT 600
        DISCOVERY_MODE PRE_TEST)

catch_discover_tests(tdfa_tests
        TEST_PREFIX "adversarial::"
        TEST_SPEC "[adversarial]~[audit]"
        DL_PATHS ${TDFA_TEST_RUNTIME_PATHS}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTIES LABELS "adversarial" TIMEOUT 180
        DISCOVERY_MODE PRE_TEST)

catch_discover_tests(tdfa_tests
        TEST_PREFIX "mutation::"
        TEST_SPEC "[mutation]~[audit]"
        DL_PATHS ${TDFA_TEST_RUNTIME_PATHS}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTIES LABELS "mutation" TIMEOUT 180
        DISCOVERY_MODE PRE_TEST)

# Body-aware coverage exercises live in a separate audit lane. They may execute
# otherwise unreachable diagnostic/error paths, but they never satisfy Routine
# contract/function traceability and never run in Fast or Deep verification.
catch_discover_tests(tdfa_tests
        TEST_PREFIX "audit::"
        TEST_SPEC "[audit]"
        DL_PATHS ${TDFA_TEST_RUNTIME_PATHS}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        PROPERTIES LABELS "audit" TIMEOUT 60 RUN_SERIAL TRUE
        DISCOVERY_MODE PRE_TEST)

add_custom_target(verify-fast
        COMMAND "${CMAKE_CTEST_COMMAND}"
                --test-dir "${CMAKE_BINARY_DIR}"
                --output-on-failure
                --no-tests=error
                --label-regex "^fast$"
        DEPENDS tdfa_tests
        USES_TERMINAL)

add_custom_target(verify-deep
        COMMAND "${CMAKE_CTEST_COMMAND}"
                --test-dir "${CMAKE_BINARY_DIR}"
                --output-on-failure
                --no-tests=error
                --label-regex "^(fast|deep)$"
        DEPENDS tdfa_tests
        USES_TERMINAL)

if(TDFA_ENABLE_SANITIZERS)
    add_custom_target(verify-adversarial
            COMMAND "${CMAKE_CTEST_COMMAND}"
                    --test-dir "${CMAKE_BINARY_DIR}"
                    --output-on-failure
                    --no-tests=error
                    --label-regex "^(fast|adversarial)$"
            DEPENDS tdfa_tests
            USES_TERMINAL)
endif()

if(TDFA_ENABLE_COVERAGE)
    find_program(TDFA_GCOVR_EXECUTABLE NAMES gcovr gcovr.exe)
    find_program(TDFA_GCOV_EXECUTABLE NAMES gcov gcov.exe)

    if(NOT TDFA_GCOVR_EXECUTABLE)
        message(FATAL_ERROR "Coverage was requested, but gcovr was not found on PATH")
    endif()
    if(NOT TDFA_GCOV_EXECUTABLE)
        message(FATAL_ERROR "Coverage was requested, but gcov was not found on PATH")
    endif()

    set(TDFA_COVERAGE_OUTPUT_DIR "${CMAKE_BINARY_DIR}/coverage")
    set(TDFA_COVERAGE_TEST_RESULT "${TDFA_COVERAGE_OUTPUT_DIR}/test-result.txt")
    add_custom_target(coverage
            COMMAND "${CMAKE_COMMAND}"
                    "-DROOT=${CMAKE_BINARY_DIR}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CleanCoverage.cmake"
            COMMAND "${CMAKE_COMMAND}" -E rm -rf "${TDFA_COVERAGE_OUTPUT_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory "${TDFA_COVERAGE_OUTPUT_DIR}"
            # Preserve the test exit code but do not stop before gcovr can emit
            # its machine-readable, LCOV, text, and HTML evidence.
            COMMAND "${CMAKE_COMMAND}"
                    "-DCTEST=${CMAKE_CTEST_COMMAND}"
                    "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                    "-DRESULT_FILE=${TDFA_COVERAGE_TEST_RESULT}"
                    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunCoverageTests.cmake"
            COMMAND "${TDFA_GCOVR_EXECUTABLE}"
                    --root "${PROJECT_SOURCE_DIR}"
                    --object-directory "${CMAKE_BINARY_DIR}"
                    --filter "${PROJECT_SOURCE_DIR}/src/"
                    --gcov-executable "${TDFA_GCOV_EXECUTABLE}"
                    --decisions
                    --print-summary
                    --lcov "${TDFA_COVERAGE_OUTPUT_DIR}/coverage.info"
                    --html-details "${TDFA_COVERAGE_OUTPUT_DIR}/index.html"
                    --json-summary "${TDFA_COVERAGE_OUTPUT_DIR}/summary.json"
                    --json-summary-pretty
                    --txt "${TDFA_COVERAGE_OUTPUT_DIR}/coverage.txt"
            DEPENDS tdfa_tests
            WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
            USES_TERMINAL
            COMMENT "Generating instrumented TDFA coverage evidence")
endif()

if(TDFA_ENABLE_FUZZING)
    file(GLOB TDFA_FUZZ_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/fuzz/*.cpp")
    set(TDFA_FUZZ_TARGETS)

    foreach(TDFA_FUZZ_SOURCE IN LISTS TDFA_FUZZ_SOURCES)
        get_filename_component(TDFA_FUZZ_NAME "${TDFA_FUZZ_SOURCE}" NAME_WE)
        set(TDFA_FUZZ_TARGET "tdfa_fuzz_${TDFA_FUZZ_NAME}")
        add_executable("${TDFA_FUZZ_TARGET}" "${TDFA_FUZZ_SOURCE}")
        target_compile_features("${TDFA_FUZZ_TARGET}" PRIVATE cxx_std_20)
        target_compile_definitions("${TDFA_FUZZ_TARGET}" PRIVATE TDFA_TESTING=1)
        target_include_directories("${TDFA_FUZZ_TARGET}" PRIVATE
                "${PROJECT_SOURCE_DIR}/tests")
        target_compile_options("${TDFA_FUZZ_TARGET}" PRIVATE
                -fsanitize=fuzzer,address,undefined
                -fno-omit-frame-pointer
                -fno-sanitize-recover=all)
        target_link_options("${TDFA_FUZZ_TARGET}" PRIVATE
                -fsanitize=fuzzer,address,undefined
                -fno-sanitize-recover=all)
        target_link_libraries("${TDFA_FUZZ_TARGET}" PRIVATE tdfa_test_subject)
        list(APPEND TDFA_FUZZ_TARGETS "${TDFA_FUZZ_TARGET}")
    endforeach()

    if(TDFA_FUZZ_TARGETS)
        set(TDFA_FUZZ_SECONDS_PER_TARGET 60 CACHE STRING
                "Seconds allocated to each target by fuzz-smoke")
        set(TDFA_FUZZ_RESULT_FILE "${CMAKE_BINARY_DIR}/fuzz-results.txt")
        set(TDFA_FUZZ_ARTIFACT_ROOT "${CMAKE_BINARY_DIR}/fuzz-artifacts")
        set(TDFA_FUZZ_CORPUS_ROOT "${CMAKE_BINARY_DIR}/fuzz-corpus")
        add_custom_target(fuzz-smoke
                COMMAND "${CMAKE_COMMAND}" -E rm -f "${TDFA_FUZZ_RESULT_FILE}"
                DEPENDS ${TDFA_FUZZ_TARGETS})
        foreach(TDFA_FUZZ_TARGET IN LISTS TDFA_FUZZ_TARGETS)
            add_custom_command(TARGET fuzz-smoke POST_BUILD
                    COMMAND "${CMAKE_COMMAND}"
                            "-DTARGET=$<TARGET_FILE:${TDFA_FUZZ_TARGET}>"
                            "-DTARGET_NAME=${TDFA_FUZZ_TARGET}"
                            "-DRESULT_FILE=${TDFA_FUZZ_RESULT_FILE}"
                            "-DARTIFACT_DIR=${TDFA_FUZZ_ARTIFACT_ROOT}/${TDFA_FUZZ_TARGET}"
                            "-DCORPUS_DIR=${TDFA_FUZZ_CORPUS_ROOT}/${TDFA_FUZZ_TARGET}"
                            "-DSECONDS=${TDFA_FUZZ_SECONDS_PER_TARGET}"
                            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunFuzzTarget.cmake"
                    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                    USES_TERMINAL)
        endforeach()
        add_custom_command(TARGET fuzz-smoke POST_BUILD
                COMMAND "${CMAKE_COMMAND}"
                        "-DRESULT_FILE=${TDFA_FUZZ_RESULT_FILE}"
                        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RequireSuccessfulFuzz.cmake"
                WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
                USES_TERMINAL)
    else()
        message(WARNING
                "TDFA_ENABLE_FUZZING is ON, but tests/fuzz contains no fuzz targets")
    endif()
endif()
