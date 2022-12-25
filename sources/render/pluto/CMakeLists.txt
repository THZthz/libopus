cmake_minimum_required(VERSION 3.3)

set(CMAKE_C_STANDARD 11) # c11 is used in plutovg

project(plutovg C)

add_library(plutovg STATIC)

add_subdirectory(source)
add_subdirectory(3rdparty/stb)
add_subdirectory(3rdparty/software)
add_subdirectory(example)
