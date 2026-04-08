add_library(vm src/vm.c)

# options
option(LUAVM_DEBUG_STACK_TRACE "Debug option to print stack on every instruction." OFF)
option(LUAVM_DEBUG_DISASSEMBLE_INSTRUCTION "Debug option to disassemble every instruction during execution." OFF)
option(LUAVM_DEBUG_DISASSEMBLE_CHUNK "Debug option to disassemble the entire chunk before execution." OFF)

if (LUAVM_DEBUG_STACK_TRACE) 
    target_compile_definitions(vm PUBLIC DEBUG_STACK_TRACE)
endif()

if (LUAVM_DEBUG_DISASSEMBLE_INSTRUCTION) 
    target_compile_definitions(vm PUBLIC DEBUG_DISASSEMBLE_INSTRUCTION)
endif()

if (LUAVM_DEBUG_DISASSEMBLE_CHUNK) 
    target_compile_definitions(vm PUBLIC DEBUG_DISASSEMBLE_CHUNK)
endif()

target_link_libraries(vm PRIVATE m)
target_link_libraries(vm PRIVATE common)
target_link_libraries(vm PRIVATE compiler)
target_link_libraries(vm PRIVATE lexer)

target_include_directories(vm PUBLIC include)
