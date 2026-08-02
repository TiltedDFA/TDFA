if(NOT DEFINED CTEST OR NOT DEFINED BUILD_DIR OR NOT DEFINED RESULT_FILE OR
   NOT DEFINED JUNIT_FILE OR NOT DEFINED OUTPUT_LOG)
    message(FATAL_ERROR
            "CTEST, BUILD_DIR, RESULT_FILE, JUNIT_FILE, and OUTPUT_LOG are required")
endif()

if(CMAKE_VERSION VERSION_LESS "3.21")
    message(FATAL_ERROR "Coverage test evidence requires CMake/CTest 3.21 or newer")
endif()

execute_process(
        COMMAND "${CTEST}"
                --test-dir "${BUILD_DIR}"
                --quiet
                --no-tests=error
                --label-regex "^(fast|deep|audit)$"
                --output-junit "${JUNIT_FILE}"
                --output-log "${OUTPUT_LOG}"
        RESULT_VARIABLE test_result)

file(WRITE "${RESULT_FILE}" "${test_result}\n")
message(STATUS "Coverage test pass completed with exit code ${test_result}; report generation will continue")
