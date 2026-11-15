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
    src/simulation/collision_grid.cpp
    src/simulation/navigation.cpp
    src/simulation/physics.cpp
    src/simulation/spatial_grid.cpp
    src/simulation/steering.cpp
)
add_library(Kinetra::Simulation ALIAS kinetra_simulation)
target_compile_features(kinetra_simulation PUBLIC cxx_std_20)
target_include_directories(kinetra_simulation PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
set_target_properties(kinetra_simulation PROPERTIES CXX_EXTENSIONS OFF)

if(KINETRA_BUILD_RUNTIME)
    find_package(SDL2 REQUIRED)
    find_package(OpenGL REQUIRED)
    add_library(kinetra_runtime src/engine/runtime.cpp src/engine/session.cpp src/glad.c)
    add_library(Kinetra::Runtime ALIAS kinetra_runtime)
    target_compile_features(kinetra_runtime PUBLIC cxx_std_20)
    target_include_directories(kinetra_runtime PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)
    target_link_libraries(kinetra_runtime PRIVATE SDL2::SDL2 OpenGL::GL)
endif()
if(BUILD_TESTING)
    add_executable(kinetra_tests tests/simulation_tests.cpp)
    target_link_libraries(kinetra_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_behavior COMMAND kinetra_tests)
    add_executable(kinetra_navigation_tests tests/navigation_tests.cpp)
    target_link_libraries(kinetra_navigation_tests PRIVATE Kinetra::Simulation)
    add_test(NAME kinetra_navigation COMMAND kinetra_navigation_tests)
    add_test(NAME kinetra_external_headless COMMAND ${CMAKE_COMMAND}
        -DKINETRA_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}
        -DCONSUMER_BINARY=${CMAKE_CURRENT_BINARY_DIR}/consumer-headless
        -DCONSUMER_GRAPHICS=OFF
        -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_consumer.cmake)
    if(KINETRA_BUILD_RUNTIME)
        add_test(NAME kinetra_external_runtime COMMAND ${CMAKE_COMMAND}
            -DKINETRA_SOURCE=${CMAKE_CURRENT_SOURCE_DIR}
            -DCONSUMER_BINARY=${CMAKE_CURRENT_BINARY_DIR}/consumer-runtime
            -DCONSUMER_GRAPHICS=ON
            "-DCONSUMER_PREFIX=${CMAKE_PREFIX_PATH}"
            "-DCONSUMER_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
            "-DCONSUMER_C_FLAGS=${CMAKE_C_FLAGS}"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_consumer.cmake)
    endif()
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    add_test(NAME kinetra_repository_checker COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/test_repository_check.py)
    add_test(NAME kinetra_repository COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_repository.py)
endif()
if(BUILD_TESTING AND KINETRA_BUILD_RUNTIME AND KINETRA_TEST_GRAPHICS)
    add_executable(kinetra_engine_runtime_tests tests/runtime_tests.cpp)
    target_link_libraries(kinetra_engine_runtime_tests PRIVATE Kinetra::Runtime SDL2::SDL2 OpenGL::GL)
    add_test(NAME kinetra_engine_graphics COMMAND kinetra_engine_runtime_tests)
    set_tests_properties(kinetra_engine_graphics PROPERTIES LABELS graphics)
endif()
# Export the same public targets for installed and source consumers.
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
set_target_properties(kinetra_simulation PROPERTIES EXPORT_NAME Simulation)
set(kinetra_install_targets kinetra_simulation)
if(KINETRA_BUILD_RUNTIME)
    set_target_properties(kinetra_runtime PROPERTIES EXPORT_NAME Runtime)
    list(APPEND kinetra_install_targets kinetra_runtime)
endif()
install(TARGETS ${kinetra_install_targets} EXPORT KinetraTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(EXPORT KinetraTargets NAMESPACE Kinetra:: DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Kinetra)
configure_package_config_file(cmake/KinetraConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/KinetraConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Kinetra)
write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/KinetraConfigVersion.cmake
    VERSION ${PROJECT_VERSION} COMPATIBILITY SameMinorVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/KinetraConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/KinetraConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Kinetra)
