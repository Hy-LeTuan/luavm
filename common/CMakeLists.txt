add_library(common utils/file_utils.c chunk/chunk.c chunk/value.c memory/memory.c)
target_include_directories(common PUBLIC utils)
target_include_directories(common PUBLIC chunk)
target_include_directories(common PUBLIC memory)
