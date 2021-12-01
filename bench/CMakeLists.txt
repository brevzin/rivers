include(FetchContent)

FetchContent_Declare(
    range-v3
    GIT_REPOSITORY https://github.com/ericniebler/range-v3.git
    GIT_TAG 83783f578e0e6666d68a3bf17b0038a80e62530e
    GIT_SHALLOW TRUE)

FetchContent_MakeAvailable(range-v3)

add_executable(benchmark EXCLUDE_FROM_ALL benchmark.cxx)
target_link_libraries(benchmark PUBLIC rivers range-v3)
