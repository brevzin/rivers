file(GLOB sources CONFIGURE_DEPENDS *.cxx)

add_executable(rivers-test ${sources})
target_link_libraries(rivers-test PUBLIC rivers)
add_test(NAME rivers-test COMMAND rivers-test)
