# Compiler warning policy shared by Kinetra and Kinetra-Lab.
#
# The warning set matches docs/conventions.md: -Wall -Wextra -Wpedantic -Wshadow
# -Wconversion, applied to C++ sources only (generated C such as GLAD is exempt).
# Kinetra applies it to its own targets; Lab applies it to its application
# directories with its own options, so each repository decides when warnings
# become errors.

include_guard(GLOBAL)

set(KINETRA_WARNING_FLAGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion)

# kinetra_target_warnings(<target> [AS_ERRORS])
#   Adds the warning set to one target's C++ compilation. AS_ERRORS also adds -Werror.
function(kinetra_target_warnings target)
    cmake_parse_arguments(arg "AS_ERRORS" "" "" ${ARGN})
    set(flags ${KINETRA_WARNING_FLAGS})
    if(arg_AS_ERRORS)
        list(APPEND flags -Werror)
    endif()
    target_compile_options(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${flags}>")
endfunction()

# kinetra_directory_warnings(<directory> [RECURSE] [AS_ERRORS])
#   Adds the warning set to every compiled target defined in <directory>, and with
#   RECURSE also in its subdirectories. Must be called after those targets exist.
#   Interface and imported targets are skipped.
function(kinetra_directory_warnings directory)
    cmake_parse_arguments(arg "RECURSE;AS_ERRORS" "" "" ${ARGN})
    set(error_flag)
    if(arg_AS_ERRORS)
        set(error_flag AS_ERRORS)
    endif()
    get_property(targets DIRECTORY "${directory}" PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS targets)
        get_target_property(type ${target} TYPE)
        if(type MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$")
            kinetra_target_warnings(${target} ${error_flag})
        endif()
    endforeach()
    if(arg_RECURSE)
        get_property(children DIRECTORY "${directory}" PROPERTY SUBDIRECTORIES)
        foreach(child IN LISTS children)
            kinetra_directory_warnings("${child}" RECURSE ${error_flag})
        endforeach()
    endif()
endfunction()
