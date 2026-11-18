# Test registration shared by Kinetra and Kinetra-Lab.
#
# Tests use doctest (vendored in third_party/doctest) and are registered with
# kinetra_add_doctest, which creates one CTest entry per TEST_CASE. kinetra_add_test
# remains for a program that cannot use doctest.
# See docs/testing.md for how to write tests.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/../third_party/doctest/cmake/doctest.cmake")

# kinetra_add_doctest(<executable> PREFIX <ctest-prefix> SOURCES <files...>
#                     [LIBRARIES <targets...>] [LABELS <labels...>] [TIMEOUT <seconds>])
#   Builds a doctest executable with a shared main and registers each test case as
#   "<ctest-prefix>:<case name>", so `ctest -R '^<ctest-prefix>'` selects the suite.
#   Every case gets the label `unit` plus LABELS, and TIMEOUT if given.
function(kinetra_add_doctest executable)
    cmake_parse_arguments(arg "" "PREFIX;TIMEOUT" "SOURCES;LIBRARIES;LABELS" ${ARGN})
    if(NOT arg_PREFIX OR NOT arg_SOURCES)
        message(FATAL_ERROR "kinetra_add_doctest(${executable}) needs PREFIX and SOURCES")
    endif()
    add_executable(${executable} ${arg_SOURCES})
    target_link_libraries(${executable} PRIVATE kinetra_doctest_main ${arg_LIBRARIES})
    doctest_discover_tests(${executable}
        TEST_PREFIX "${arg_PREFIX}:"
        TEST_LIST ${executable}_TESTS)
    # doctest forwards PROPERTIES as a flat argument list, which splits a property with
    # several values (two labels become a label and a bogus property). Set the properties
    # in a CTest include that runs after discovery, on the list of discovered cases.
    set(labels unit ${arg_LABELS})
    set(properties "LABELS \"${labels}\"")
    if(arg_TIMEOUT)
        string(APPEND properties " TIMEOUT ${arg_TIMEOUT}")
    endif()
    set(properties_file "${CMAKE_CURRENT_BINARY_DIR}/${executable}_properties.cmake")
    file(WRITE "${properties_file}"
        "if(DEFINED ${executable}_TESTS)\n"
        "  set_tests_properties(\${${executable}_TESTS} PROPERTIES ${properties})\n"
        "endif()\n")
    set_property(DIRECTORY APPEND PROPERTY TEST_INCLUDE_FILES "${properties_file}")
endfunction()

# kinetra_add_test(<executable> NAME <ctest-name> SOURCES <files...>
#                  [LIBRARIES <targets...>] [LABELS <labels...>])
#   Builds a plain test program whose exit status is the result, and registers it
#   as one CTest entry.
function(kinetra_add_test executable)
    cmake_parse_arguments(arg "" "NAME" "SOURCES;LIBRARIES;LABELS" ${ARGN})
    if(NOT arg_NAME OR NOT arg_SOURCES)
        message(FATAL_ERROR "kinetra_add_test(${executable}) needs NAME and SOURCES")
    endif()
    add_executable(${executable} ${arg_SOURCES})
    target_link_libraries(${executable} PRIVATE ${arg_LIBRARIES})
    add_test(NAME ${arg_NAME} COMMAND ${executable})
    if(arg_LABELS)
        set_tests_properties(${arg_NAME} PROPERTIES LABELS "${arg_LABELS}")
    endif()
endfunction()
