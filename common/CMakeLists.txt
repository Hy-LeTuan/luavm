add_library(common
    chunk/chunk.c;
    chunk/value.c;
    memory/memory.c;
    memory/gc.c
    utils/file_utils.c;
    utils/disassemble.c;
    object/object.c;
    table/table.c;
    table/hash.c
)

target_include_directories(common PUBLIC utils)
target_include_directories(common PUBLIC chunk)
target_include_directories(common PUBLIC memory)
target_include_directories(common PUBLIC object)
target_include_directories(common PUBLIC table)
