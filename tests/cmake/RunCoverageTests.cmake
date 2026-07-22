if(NOT DEFINED CTEST OR NOT DEFINED BUILD_DIR OR NOT DEFINED RESULT_FILE)
    message(FATAL_ERROR "CTEST, BUILD_DIR, and RESULT_FILE are required")
endif()

execute_process(
        COMMAND "${CTEST}"
                --test-dir "${BUILD_DIR}"
                --output-on-failure
                --no-tests=error
                --label-regex "^(fast|deep|audit)$"
        RESULT_VARIABLE test_result)

file(WRITE "${RESULT_FILE}" "${test_result}\n")
message(STATUS "Coverage test pass completed with exit code ${test_result}; report generation will continue")
