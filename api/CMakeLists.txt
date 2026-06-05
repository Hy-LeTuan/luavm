add_library(luavmapi lapi.c)

target_link_libraries(luavmapi PUBLIC compiler)
target_link_libraries(luavmapi PUBLIC common)
target_link_libraries(luavmapi PUBLIC vm)
target_link_libraries(luavmapi PUBLIC lib)

target_include_directories(luavmapi PUBLIC include)
