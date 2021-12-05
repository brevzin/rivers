file(GLOB sources CONFIGURE_DEPENDS *.cxx)

if(NOT RVR_IMPORT_FMT)
    list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/fmt.cxx)
endif()

add_executable(rivers-test ${sources})
target_link_libraries(rivers-test PUBLIC rivers)
add_test(NAME rivers-test COMMAND rivers-test)
