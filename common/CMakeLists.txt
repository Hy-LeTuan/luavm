add_library(common chunk/chunk.c chunk/value.c memory/memory.c utils/file_utils.c utils/disassemble.c)

target_include_directories(common PUBLIC utils)
target_include_directories(common PUBLIC chunk)
target_include_directories(common PUBLIC memory)
