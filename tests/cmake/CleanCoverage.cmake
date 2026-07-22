if(NOT DEFINED ROOT OR ROOT STREQUAL "")
    message(FATAL_ERROR "CleanCoverage.cmake requires -DROOT=<build directory>")
endif()

get_filename_component(COVERAGE_ROOT "${ROOT}" ABSOLUTE)
if(COVERAGE_ROOT STREQUAL "/" OR COVERAGE_ROOT MATCHES "^[A-Za-z]:/$")
    message(FATAL_ERROR "Refusing to clean coverage data from filesystem root")
endif()

file(GLOB_RECURSE COVERAGE_DATA_FILES
        "${COVERAGE_ROOT}/*.gcda"
        "${COVERAGE_ROOT}/*.gcov")

if(COVERAGE_DATA_FILES)
    file(REMOVE ${COVERAGE_DATA_FILES})
endif()
