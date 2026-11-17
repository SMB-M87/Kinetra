cmake_minimum_required(VERSION 3.16)
project(Kinetra VERSION 0.1.0 LANGUAGES C CXX)

option(KINETRA_BUILD_RUNTIME "Build SDL2/OpenGL runtime" OFF)
option(KINETRA_TEST_GRAPHICS "Enable graphics tests (requires OpenGL context)" OFF)
include(CTest)

# Do not change an embedding project's output paths.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
endif()
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
    add_library(kinetra_runtime src/engine/runtime.cpp src/engine/session.cpp src/glad.c)
    add_library(Kinetra::Runtime ALIAS kinetra_runtime)
    target_compile_features(kinetra_runtime PUBLIC cxx_std_20)
    target_include_directories(kinetra_runtime PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_link_libraries(kinetra_runtime PRIVATE SDL2::SDL2 OpenGL::GL)
    add_library(kinetra_launcher src/engine/launcher.cpp)
    add_library(Kinetra::Launcher ALIAS kinetra_launcher)
    target_link_libraries(kinetra_launcher PUBLIC Kinetra::Runtime)
    if(BUILD_TESTING)
        add_executable(kinetra_launcher_tests tests/launcher_tests.cpp)
        target_link_libraries(kinetra_launcher_tests PRIVATE Kinetra::Launcher)
        add_test(NAME kinetra_launcher COMMAND kinetra_launcher_tests)
    endif()
endif()
if(BUILD_TESTING)
    add_executable(kinetra_traffic_tests tests/traffic_tests.cpp)
    target_link_libraries(kinetra_traffic_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_traffic COMMAND kinetra_traffic_tests)
    add_executable(kinetra_avoidance_tests tests/avoidance_tests.cpp)
    target_link_libraries(kinetra_avoidance_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_avoidance COMMAND kinetra_avoidance_tests)
    add_executable(kinetra_moving_contact_tests tests/moving_contact_tests.cpp)
    target_link_libraries(kinetra_moving_contact_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_moving_contacts COMMAND kinetra_moving_contact_tests)
    add_executable(kinetra_translation_sweep_tests tests/translation_sweep_tests.cpp)
    target_link_libraries(kinetra_translation_sweep_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_translation_sweeps COMMAND kinetra_translation_sweep_tests)
    add_executable(kinetra_sweep_tests tests/sweep_tests.cpp)
    target_link_libraries(kinetra_sweep_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_sweeps COMMAND kinetra_sweep_tests)
    add_executable(kinetra_polygon_tests tests/polygon_tests.cpp)
    target_link_libraries(kinetra_polygon_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_polygons COMMAND kinetra_polygon_tests)
    add_executable(kinetra_weighted_navigation_tests tests/weighted_navigation_tests.cpp)
    target_link_libraries(kinetra_weighted_navigation_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_weighted_navigation COMMAND kinetra_weighted_navigation_tests)
    add_executable(kinetra_tests tests/simulation_tests.cpp)
    target_link_libraries(kinetra_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_behavior COMMAND kinetra_tests)
    add_executable(kinetra_navigation_tests tests/navigation_tests.cpp)
    target_link_libraries(kinetra_navigation_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_navigation COMMAND kinetra_navigation_tests)
endif()
if(BUILD_TESTING AND KINETRA_BUILD_RUNTIME AND KINETRA_TEST_GRAPHICS)
    add_executable(kinetra_engine_runtime_tests tests/runtime_tests.cpp)
    target_link_libraries(kinetra_engine_runtime_tests PRIVATE Kinetra::Runtime SDL2::SDL2 OpenGL::GL)
    add_test(NAME kinetra_engine_graphics COMMAND kinetra_engine_runtime_tests)
    set_tests_properties(kinetra_engine_graphics PROPERTIES LABELS graphics)
endif()
