add_library(compiler compiler.c)

target_link_libraries(compiler PRIVATE common)
target_link_libraries(compiler PRIVATE lexer)

target_include_directories(compiler PUBLIC include)
