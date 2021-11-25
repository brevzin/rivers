cmake_minimum_required(VERSION 3.0.0)
project(rivers VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(rivers INTERFACE)
add_library(rivers::rivers ALIAS rivers)
target_include_directories(rivers INTERFACE include)

add_subdirectory(test)
