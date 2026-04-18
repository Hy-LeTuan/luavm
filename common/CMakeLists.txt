add_library(common
    #chunks
    chunk/chunk.c;
    chunk/value.c;

    memory/memory.c;
    memory/gc.c
    utils/file_utils.c;
    utils/disassemble.c;

    # objects
    object/object.c;
    object/objstring.c;

    # table
    table/table.c;
    table/hash.c

    # var
    var/locals.c
)

target_include_directories(common PUBLIC utils)
target_include_directories(common PUBLIC chunk)
target_include_directories(common PUBLIC memory)
target_include_directories(common PUBLIC object)
target_include_directories(common PUBLIC table)
target_include_directories(common PUBLIC var)
