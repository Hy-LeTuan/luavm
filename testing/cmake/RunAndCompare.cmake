message(STATUS "Running: ${TEST_EXECUTABLE} ${INPUT_FILE}")

execute_process(COMMAND ${TEST_EXECUTABLE} ${INPUT_FILE}
    OUTPUT_FILE "${ACTUAL_OUTPUT_PATH}"
    ERROR_VARIABLE ERROR_OUTPUT
    RESULT_VARIABLE EXIT_CODE
)

if (NOT EXIT_CODE EQUAL 0)
    message(FATAL_ERROR "Interpreter crashed with error: ${ERROR_OUTPUT}")
endif()

# Compare the actual output with the expected "Golden" file
execute_process(
    COMMAND ${CMAKE_COMMAND} -E compare_files --ignore-eol "${ACTUAL_OUTPUT_PATH}" "${EXPECTED_OUTPUT_PATH}"
    RESULT_VARIABLE DIFF_RESULT
)

if(NOT DIFF_RESULT EQUAL 0)
    message("Output mismatch!")
    message(FATAL_ERROR "Difference between ${ACTUAL_OUTPUT_PATH} and ${EXPECTED_OUTPUT_PATH}")
endif()
