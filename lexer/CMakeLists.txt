add_library(lexer lexer.c)

option(LUAVM_DEBUG_PRINT_TOKEN "Debug option to print tokens lexed during test." OFF)
if (LUAVM_DEBUG_PRINT_TOKEN) 
    target_compile_definitions(lexer PUBLIC DEBUG_PRINT_TOKEN)
endif()

target_include_directories(lexer PUBLIC include)
