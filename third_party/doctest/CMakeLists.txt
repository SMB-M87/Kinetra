# Vendored doctest (see README.md). kinetra_doctest provides the header as a
# system include, so first-party warning flags do not apply to it.
add_library(kinetra_doctest INTERFACE)
target_include_directories(kinetra_doctest SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(kinetra_doctest INTERFACE cxx_std_20)

# Shared main() for every doctest executable; built only when a test links it.
add_library(kinetra_doctest_main STATIC EXCLUDE_FROM_ALL doctest_main.cpp)
target_link_libraries(kinetra_doctest_main PUBLIC kinetra_doctest)
