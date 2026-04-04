include(CTest)

# test token production from lexer
add_executable(TokenLex lexer/token_test.c)
add_test(NAME TokenLex COMMAND TokenLex ${CMAKE_SOURCE_DIR}/testing/data/tokens.lua)
target_link_libraries(TokenLex PRIVATE lexer)
target_link_libraries(TokenLex PRIVATE common)

# test operations on chunk
add_executable(ChunkOps compiler/chunk_test.c)
add_test(NAME ChunkOps COMMAND ChunkOps)
target_link_libraries(ChunkOps PRIVATE common)
