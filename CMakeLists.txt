cmake_minimum_required(VERSION 3.16)
project(compiler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(compiler
    main.cpp
    token.cpp
    variable.cpp
    asm_command.cpp
    compiler.cpp
)

target_include_directories(compiler PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
