include(CTest)

# PRE-EXECUTION TEST

# test token production from lexer
add_executable(TokenLexTest lexer/token_test.c)
add_test(NAME TokenLexTest COMMAND TokenLexTest ${CMAKE_SOURCE_DIR}/testing/data/token_input.lua)
target_link_libraries(TokenLexTest PRIVATE lexer)
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

# test expressions
add_executable(ExpressionTest function/expression_test.c)
target_link_libraries(ExpressionTest PRIVATE common)
target_link_libraries(ExpressionTest PRIVATE lexer)
target_link_libraries(ExpressionTest PRIVATE compiler)
target_link_libraries(ExpressionTest PRIVATE vm)
add_test(NAME ExpressionTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:ExpressionTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/expression_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/unit/expression_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/expression_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test goto statements
add_executable(JumpTest function/jump_test.c)
target_link_libraries(JumpTest PRIVATE common)
target_link_libraries(JumpTest PRIVATE lexer)
target_link_libraries(JumpTest PRIVATE compiler)
target_link_libraries(JumpTest PRIVATE vm)
add_test(NAME JumpTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:JumpTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/jump_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/unit/jump_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/jump_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test function definitions and calls
add_executable(FunctionTest function/function_test.c)
target_link_libraries(FunctionTest PRIVATE common)
target_link_libraries(FunctionTest PRIVATE lexer)
target_link_libraries(FunctionTest PRIVATE compiler)
target_link_libraries(FunctionTest PRIVATE vm)
add_test(NAME FunctionTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:FunctionTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/function_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/unit/function_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/function_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)

# test recursive function and upvalues
add_executable(RecursionTest function/recursion_test.c)
target_link_libraries(RecursionTest PRIVATE common)
target_link_libraries(RecursionTest PRIVATE lexer)
target_link_libraries(RecursionTest PRIVATE compiler)
target_link_libraries(RecursionTest PRIVATE vm)
add_test(NAME RecursionTest
    COMMAND ${CMAKE_COMMAND}
    -D "TEST_EXECUTABLE=$<TARGET_FILE:RecursionTest>"
    -D "INPUT_FILE=${CMAKE_SOURCE_DIR}/testing/data/recursion_input.lua"
    -D "EXPECTED_OUTPUT_PATH=${CMAKE_SOURCE_DIR}/testing/data/unit/recursion_test.expected"
    -D "ACTUAL_OUTPUT_PATH=${CMAKE_CURRENT_BINARY_DIR}/recursion_test.actual"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunAndCompare.cmake"
)
