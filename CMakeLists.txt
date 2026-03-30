cmake_minimum_required(VERSION 4.2)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(luavm VERSION 1.0 LANGUAGES C)

set(PROJECT_SRC src/main.c lexer/lexer.c compiler/compiler.c vm/vm.c)
add_executable(${PROJECT_NAME} ${PROJECT_SRC})

target_include_directories(${PROJECT_NAME} PRIVATE lexer)
target_include_directories(${PROJECT_NAME} PRIVATE compiler)
target_include_directories(${PROJECT_NAME} PRIVATE vm)
