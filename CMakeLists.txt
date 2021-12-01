cmake_minimum_required(VERSION 3.0.0)
project(rivers VERSION 0.1.0)

include(CTest)
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(rivers INTERFACE)
add_library(rivers::rivers ALIAS rivers)
target_include_directories(rivers INTERFACE include)

option(RVR_IMPORT_FMT Off)
if(RVR_IMPORT_FMT)
    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 713c7c7c62044329d26c19323ffa0e64d69d2d64
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(fmt)

    target_link_libraries(rivers INTERFACE fmt::fmt)
endif()

add_subdirectory(bench)
add_subdirectory(test)
