add_library(common
    #chunks
    chunk/chunk.c;

    memory/memory.c;
    memory/gc.c
    utils/file_utils.c;
    utils/disassemble.c;

    # objects
    object/value.c
    object/object.c;
    object/objstring.c;
    object/objfunction.c
    object/objclosure.c
    object/objupvalue.c
    object/objnativefunction.c
    object/objtable.c

    # table
    table/table.c;
    table/hash.c

    # var
    var/reserved_var.c
)

option(LUAVM_DEBUG_STRESS_GC "Debug option to always activate GC on memory allocation." OFF)
option(LUAVM_DEBUG_LOG_GC "Debug option to log GC activities." OFF)

if (LUAVM_DEBUG_STRESS_GC)
    target_compile_definitions(common PUBLIC DEBUG_STRESS_GC)
endif()

if (LUAVM_DEBUG_LOG_GC)
    target_compile_definitions(common PUBLIC DEBUG_LOG_GC)
endif()

target_link_libraries(common PRIVATE m)

target_include_directories(common PUBLIC utils)
target_include_directories(common PUBLIC chunk)
target_include_directories(common PUBLIC memory)
target_include_directories(common PUBLIC object)
target_include_directories(common PUBLIC table)
target_include_directories(common PUBLIC var)
target_include_directories(common PUBLIC metatable)
target_include_directories(common PUBLIC state)
