add_library(lib
    base/baselib.c
    string/stringlib.c
    table/tablelib.c
    math/mathlib.c
)

target_link_libraries(lib PRIVATE common)
target_link_libraries(lib PRIVATE vm)
target_link_libraries(lib PRIVATE m)

target_include_directories(lib PUBLIC base)
target_include_directories(lib PUBLIC string)
target_include_directories(lib PUBLIC table)
target_include_directories(lib PUBLIC math)
target_include_directories(lib PUBLIC utils)
