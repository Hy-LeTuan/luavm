include(CTest)

# test token production from lexer
add_executable(TokenLexTest lexer/token_test.c)
add_test(NAME TokenLexTest COMMAND TokenLexTest ${CMAKE_SOURCE_DIR}/testing/data/token_input.lua)
target_link_libraries(TokenLexTest PRIVATE lexer)
target_link_libraries(TokenLexTest PRIVATE common)

# test operations on chunk
add_executable(ChunkOpsTest compiler/chunk_test.c)
add_test(NAME ChunkOpsTest COMMAND ChunkOpsTest)
target_link_libraries(ChunkOpsTest PRIVATE common)

# test calculations 
add_executable(CalcTest function/calculation_test.c)
add_test(NAME CalcTest COMMAND CalcTest ${CMAKE_SOURCE_DIR}/testing/data/calculations.lua)
target_link_libraries(CalcTest PRIVATE common)
target_link_libraries(CalcTest PRIVATE lexer)
target_link_libraries(CalcTest PRIVATE compiler)
target_link_libraries(CalcTest PRIVATE vm)

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

# test local variable
add_executable(LocalVarTest function/local_var_test.c)
add_test(NAME LocalVarTest COMMAND LocalVarTest ${CMAKE_SOURCE_DIR}/testing/data/local_var.lua)
target_link_libraries(LocalVarTest PRIVATE common)
target_link_libraries(LocalVarTest PRIVATE lexer)
target_link_libraries(LocalVarTest PRIVATE compiler)
target_link_libraries(LocalVarTest PRIVATE vm)

# test boolean
add_executable(BooleanTest function/boolean_test.c)
add_test(NAME BooleanTest COMMAND BooleanTest ${CMAKE_SOURCE_DIR}/testing/data/boolean_input.lua)
target_link_libraries(BooleanTest PRIVATE common)
target_link_libraries(BooleanTest PRIVATE lexer)
target_link_libraries(BooleanTest PRIVATE compiler)
target_link_libraries(BooleanTest PRIVATE vm)

# test jump
add_executable(JumpTest function/jump_test.c)
add_test(NAME JumpTest COMMAND JumpTest ${CMAKE_SOURCE_DIR}/testing/data/jump_input.lua)
target_link_libraries(JumpTest PRIVATE common)
target_link_libraries(JumpTest PRIVATE lexer)
target_link_libraries(JumpTest PRIVATE compiler)
target_link_libraries(JumpTest PRIVATE vm)

# test function definitions and calls
add_executable(CallTest function/call_test.c)
add_test(NAME CallTest COMMAND CallTest ${CMAKE_SOURCE_DIR}/testing/data/call_input.lua)
target_link_libraries(CallTest PRIVATE common)
target_link_libraries(CallTest PRIVATE lexer)
target_link_libraries(CallTest PRIVATE compiler)
target_link_libraries(CallTest PRIVATE vm)
