cmake_minimum_required(VERSION 3.16)

project(komp VERSION 0.1.0 LANGUAGES CXX)

add_executable(komp
    main.cpp
    compiler.cpp
    token.cpp
    tree.cpp
    asm_command.cpp
    varible.cpp
    mask.cpp
)

target_compile_features(komp PRIVATE cxx_std_17)

target_include_directories(komp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
