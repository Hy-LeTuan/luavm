include(CTest)

# PRE-EXECUTION TEST

# test token production from lexer
add_executable(TokenLexTest lexer/token_test.c)
add_test(NAME TokenLexTest COMMAND TokenLexTest ${CMAKE_SOURCE_DIR}/testing/data/token_test_input.lua)
target_link_libraries(TokenLexTest PRIVATE compiler)
target_link_libraries(TokenLexTest PRIVATE common)

# test operations on chunk
add_executable(ChunkOpsTest compiler/chunk_test.c)
add_test(NAME ChunkOpsTest COMMAND ChunkOpsTest)
target_link_libraries(ChunkOpsTest PRIVATE common)

# test hashing
add_executable(HashTest function/hash_test.c)
add_test(NAME HashTest COMMAND HashTest)
target_link_libraries(HashTest PRIVATE common)
target_link_libraries(HashTest PRIVATE vm)

# test table
add_executable(TableTest function/table_test.c)
add_test(NAME TableTest COMMAND TableTest)
target_link_libraries(TableTest PRIVATE common)
target_link_libraries(TableTest PRIVATE vm)

# test string
add_executable(StringTest function/string_test.c)
add_test(NAME StringTest COMMAND StringTest)
target_link_libraries(StringTest PRIVATE common)
target_link_libraries(StringTest PRIVATE vm)

# EXECUTION TEST

# test objtable basic operations
add_executable(ObjtableTest function/objtable_test.c)
target_link_libraries(ObjtableTest PRIVATE luavmapi)
add_test(NAME ObjtableTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:ObjtableTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/objtable_test_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/expected/objtable_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/objtable_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test expressions
add_executable(ExpressionTest function/expression_test.c)
target_link_libraries(ExpressionTest PRIVATE luavmapi)
add_test(NAME ExpressionTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:ExpressionTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/expression_test_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/expected/expression_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/expression_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test goto statements
add_executable(JumpTest function/jump_test.c)
target_link_libraries(JumpTest PRIVATE luavmapi)
add_test(NAME JumpTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:JumpTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/jump_test_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/expected/jump_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/jump_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test assignment statements in local and in varlist
add_executable(AssignTest function/assign_test.c)
target_link_libraries(AssignTest PRIVATE luavmapi)
add_test(NAME AssignTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:AssignTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/assign_test_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/expected/assign_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/assign_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test function definitions and calls
add_executable(FunctionTest function/function_test.c)
target_link_libraries(FunctionTest PRIVATE luavmapi)
add_test(NAME FunctionTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:FunctionTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/function_test_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/expected/function_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/function_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test recursive function and upvalues
add_executable(ClosureAndIterTest function/closure_and_iter_test.c)
target_link_libraries(ClosureAndIterTest PRIVATE luavmapi)
add_test(NAME ClosureAndIterTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:ClosureAndIterTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/closure_and_iter_test_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/expected/closure_and_iter_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/closure_and_iter_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test multret expressions and multret adjustment
add_executable(MultretTest function/multret_test.c)
target_link_libraries(MultretTest PRIVATE luavmapi)
add_test(NAME MultretTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:MultretTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/multret_test_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/expected/multret_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/multret_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)
