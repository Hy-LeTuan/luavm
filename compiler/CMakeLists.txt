add_library(compiler lexer.c compiler.c)

target_link_libraries(compiler PRIVATE common)

target_include_directories(compiler PUBLIC include)
