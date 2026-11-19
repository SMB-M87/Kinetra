# Windows executable icon. Runtime gives every window the Kinetra icon at run time;
# this module also embeds it in the .exe, so Explorer, shortcuts and the taskbar show
# it before a window opens. The icon comes from tools/make_icon.py.
#
# kinetra_executable_icon(<target>) adds the icon resource to an executable target when
# building for Windows and does nothing elsewhere. Kinetra-Lab calls it for its
# applications. CMake enables the RC language itself for the MinGW and MSVC platforms.

if(WIN32)
    get_filename_component(KINETRA_ICON_FILE "${CMAKE_CURRENT_LIST_DIR}/../resources/kinetra.ico" ABSOLUTE)
    # The resource compiler resolves the icon path relative to its working directory,
    # so the generated script names it absolutely.
    # Cached, so the function also finds it when called from an embedding project's
    # directories, outside this one's variable scope.
    set(KINETRA_ICON_RESOURCE "${CMAKE_CURRENT_BINARY_DIR}/kinetra_icon.rc" CACHE INTERNAL "Kinetra icon resource")
    configure_file("${CMAKE_CURRENT_LIST_DIR}/../resources/kinetra.rc.in" "${KINETRA_ICON_RESOURCE}" @ONLY)
endif()

function(kinetra_executable_icon target)
    if(WIN32)
        target_sources(${target} PRIVATE "${KINETRA_ICON_RESOURCE}")
    endif()
endfunction()
