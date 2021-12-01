file(GLOB sources CONFIGURE_DEPENDS *.cxx)

set(sources
    main.cxx
    split.cxx
    value_type.cxx)

if(RVR_IMPORT_FMT)
    list(APPEND sources fmt.cxx)
endif()

add_executable(rivers-test ${sources})
target_link_libraries(rivers-test PUBLIC rivers)
add_test(NAME rivers-test COMMAND rivers-test)
