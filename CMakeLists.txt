cmake_minimum_required(VERSION 3.16)
project(Kinetra VERSION 0.1.0 LANGUAGES C CXX)

option(KINETRA_BUILD_RUNTIME "Build SDL2/OpenGL runtime" OFF)
option(KINETRA_TEST_GRAPHICS "Enable graphics tests (requires OpenGL context)" OFF)
option(KINETRA_WARNINGS "Compile Kinetra targets with the extended warning set" OFF)
option(KINETRA_WARNINGS_AS_ERRORS "Treat Kinetra warnings as errors (requires KINETRA_WARNINGS)" OFF)
include(CTest)
include(cmake/KinetraWarnings.cmake)

# Do not change an embedding project's output paths.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

# Test support is defined even when this project's tests are off, so Kinetra-Lab
# can use doctest and the registration helpers through add_subdirectory.
add_subdirectory(third_party/doctest)
include(cmake/KinetraTesting.cmake)

add_library(kinetra_simulation
    src/simulation/traffic.cpp
    src/simulation/avoidance.cpp
    src/simulation/collision_grid.cpp
    src/simulation/navigation.cpp
    src/simulation/physics.cpp
    src/simulation/spatial_grid.cpp
    src/simulation/priority_steering.cpp
    src/simulation/weighted_grid.cpp
    src/simulation/steering.cpp
)
add_library(Kinetra::Simulation ALIAS kinetra_simulation)
target_compile_features(kinetra_simulation PUBLIC cxx_std_20)
target_include_directories(kinetra_simulation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include)
set_target_properties(kinetra_simulation PROPERTIES CXX_EXTENSIONS OFF)

if(KINETRA_BUILD_RUNTIME)
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
    add_library(kinetra_runtime
        src/engine/runtime.cpp
        src/engine/session.cpp
        src/engine/settings_view.cpp
        src/glad.c)
    add_library(Kinetra::Runtime ALIAS kinetra_runtime)
    target_compile_features(kinetra_runtime PUBLIC cxx_std_20)
    target_include_directories(kinetra_runtime PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(kinetra_runtime PRIVATE SDL2::SDL2 OpenGL::GL)
    add_library(kinetra_launcher src/engine/launcher.cpp)
    add_library(Kinetra::Launcher ALIAS kinetra_launcher)
    target_compile_features(kinetra_launcher PUBLIC cxx_std_20)
    target_link_libraries(kinetra_launcher PUBLIC Kinetra::Runtime)
    if(BUILD_TESTING)
        # Window-free: these suites link the runtime libraries but never open a window.
        kinetra_add_doctest(kinetra_launcher_tests PREFIX kinetra_launcher
            SOURCES tests/launcher_tests.cpp LIBRARIES Kinetra::Launcher)
        kinetra_add_doctest(kinetra_settings_view_tests PREFIX kinetra_settings_view
            SOURCES tests/settings_view_tests.cpp LIBRARIES Kinetra::Runtime)
    endif()
endif()

if(BUILD_TESTING)
    kinetra_add_doctest(kinetra_traffic_tests PREFIX kinetra_traffic
        SOURCES tests/traffic_tests.cpp LIBRARIES Kinetra::Simulation)
    # The glyph table and the camera and timing helpers are header-only and need no
    # runtime; the Simulation target only supplies the include directory here.
    kinetra_add_doctest(kinetra_presentation_tests PREFIX kinetra_presentation
        SOURCES tests/presentation_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_avoidance_tests PREFIX kinetra_avoidance
        SOURCES tests/avoidance_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_moving_contact_tests PREFIX kinetra_moving_contacts
        SOURCES tests/moving_contact_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_translation_sweep_tests PREFIX kinetra_translation_sweeps
        SOURCES tests/translation_sweep_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_sweep_tests PREFIX kinetra_sweeps
        SOURCES tests/sweep_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_polygon_tests PREFIX kinetra_polygons
        SOURCES tests/polygon_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_weighted_navigation_tests PREFIX kinetra_weighted_navigation
        SOURCES tests/weighted_navigation_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_navigation_tests PREFIX kinetra_navigation
        SOURCES tests/navigation_tests.cpp LIBRARIES Kinetra::Simulation)
    kinetra_add_doctest(kinetra_tests PREFIX kinetra_behavior
        SOURCES tests/simulation_tests.cpp LIBRARIES Kinetra::Simulation)
endif()
if(BUILD_TESTING AND KINETRA_BUILD_RUNTIME AND KINETRA_TEST_GRAPHICS)
    kinetra_add_doctest(kinetra_engine_runtime_tests PREFIX kinetra_engine_graphics
        SOURCES tests/runtime_tests.cpp LIBRARIES Kinetra::Launcher SDL2::SDL2 OpenGL::GL
        LABELS graphics)
endif()

# Applies to the targets above only; vendored third-party targets are exempt.
if(KINETRA_WARNINGS)
    if(KINETRA_WARNINGS_AS_ERRORS)
        kinetra_directory_warnings(${CMAKE_CURRENT_SOURCE_DIR} AS_ERRORS)
    else()
        kinetra_directory_warnings(${CMAKE_CURRENT_SOURCE_DIR})
    endif()
endif()
