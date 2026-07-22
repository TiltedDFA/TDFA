if(NOT DEFINED TARGET OR NOT DEFINED TARGET_NAME OR NOT DEFINED RESULT_FILE OR
   NOT DEFINED ARTIFACT_DIR OR NOT DEFINED CORPUS_DIR OR NOT DEFINED SECONDS)
    message(FATAL_ERROR
            "TARGET, TARGET_NAME, RESULT_FILE, ARTIFACT_DIR, CORPUS_DIR, and SECONDS are required")
endif()

file(MAKE_DIRECTORY "${ARTIFACT_DIR}")
file(MAKE_DIRECTORY "${CORPUS_DIR}")

# A fixed-duration campaign is not a regression test by itself: it may never
# rediscover a prior minimized crash. Replay every retained artifact first,
# recording each result independently, then run the ordinary corpus campaign.
file(GLOB retained_artifacts LIST_DIRECTORIES FALSE "${ARTIFACT_DIR}/*")
foreach(artifact IN LISTS retained_artifacts)
    get_filename_component(artifact_name "${artifact}" NAME)
    execute_process(
            COMMAND "${TARGET}"
                    -runs=1
                    -timeout=10
                    -max_len=64
                    -seed=1413760577
                    -print_final_stats=1
                    "-artifact_prefix=${ARTIFACT_DIR}/"
                    "${artifact}"
            RESULT_VARIABLE replay_result)
    file(APPEND "${RESULT_FILE}"
            "${TARGET_NAME}::replay::${artifact_name}=${replay_result}\n")
endforeach()

execute_process(
        COMMAND "${TARGET}"
                "-max_total_time=${SECONDS}"
                -timeout=10
                -max_len=64
                -seed=1413760577
                -print_final_stats=1
                "-artifact_prefix=${ARTIFACT_DIR}/"
                "${CORPUS_DIR}"
        RESULT_VARIABLE fuzz_result)
file(APPEND "${RESULT_FILE}" "${TARGET_NAME}::campaign=${fuzz_result}\n")
message(STATUS "${TARGET_NAME} completed with exit code ${fuzz_result}")
