cmake_minimum_required(VERSION 4.2)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(luavm VERSION 1.0 LANGUAGES C)

set(PROJECT_SRC src/main.c)
add_executable(${PROJECT_NAME} ${PROJECT_SRC})


# add subdirectories
add_subdirectory(common)
target_link_libraries(${PROJECT_NAME} PRIVATE common)

add_subdirectory(lexer)
target_link_libraries(${PROJECT_NAME} PRIVATE lexer)

add_subdirectory(compiler)
target_link_libraries(${PROJECT_NAME} PRIVATE compiler)

add_subdirectory(vm)
target_link_libraries(${PROJECT_NAME} PRIVATE vm)

enable_testing()
add_subdirectory(testing)
