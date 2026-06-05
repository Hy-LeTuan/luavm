add_library(lib
    base/baselib.c
)

target_link_libraries(lib PRIVATE common)
target_link_libraries(lib PRIVATE vm)

target_include_directories(lib PUBLIC base)
