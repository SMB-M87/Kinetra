# Test registration shared by Kinetra and Kinetra-Lab.
#
# New tests use doctest (vendored in third_party/doctest) and are registered with
# kinetra_add_doctest, which creates one CTest entry per TEST_CASE. Existing plain
# main()-style tests use kinetra_add_test until they are converted.
# See docs/testing.md for how to write tests.

include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/../third_party/doctest/cmake/doctest.cmake")

# kinetra_add_doctest(<executable> PREFIX <ctest-prefix> SOURCES <files...>
#                     [LIBRARIES <targets...>] [LABELS <labels...>])
#   Builds a doctest executable with a shared main and registers each test case as
#   "<ctest-prefix>:<case name>", so `ctest -R '^<ctest-prefix>'` selects the suite.
function(kinetra_add_doctest executable)
    cmake_parse_arguments(arg "" "PREFIX" "SOURCES;LIBRARIES;LABELS" ${ARGN})
    if(NOT arg_PREFIX OR NOT arg_SOURCES)
        message(FATAL_ERROR "kinetra_add_doctest(${executable}) needs PREFIX and SOURCES")
    endif()
    add_executable(${executable} ${arg_SOURCES})
    target_link_libraries(${executable} PRIVATE kinetra_doctest_main ${arg_LIBRARIES})
    set(labels unit ${arg_LABELS})
    doctest_discover_tests(${executable}
        TEST_PREFIX "${arg_PREFIX}:"
        PROPERTIES LABELS "${labels}")
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
