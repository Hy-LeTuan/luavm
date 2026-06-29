add_library(lib
    base/baselib.c
    string/stringlib.c
    table/tablelib.c
    math/mathlib.c
)

option(LUAVM_DEBUG_REQUIRE "Debug option to make require on a variety of paths." OFF)

if (LUAVM_DEBUG_REQUIRE)
    target_compile_definitions(lib PUBLIC DEBUG_REQUIRE)
endif()

target_link_libraries(lib PRIVATE common)
target_link_libraries(lib PRIVATE vm)
target_link_libraries(lib PRIVATE compiler)
target_link_libraries(lib PRIVATE m)

target_include_directories(lib PUBLIC base)
target_include_directories(lib PUBLIC string)
target_include_directories(lib PUBLIC table)
target_include_directories(lib PUBLIC math)
target_include_directories(lib PUBLIC utils)
