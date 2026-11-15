# Development and verification

Core builds need no Kinetra-Lab checkout. Application builds and tests belong to Lab.

## Table of contents

- [Dependencies and options](#dependencies-and-options)
- [Checks](#checks)
- [Graphics execution](#graphics-execution)
- [External consumers](#external-consumers)

## Dependencies and options

C++20, CMake 3.16+ and a build tool are required. Python 3 runs repository checks. Runtime additionally requires SDL2/OpenGL development files and an OpenGL 3.3 context.

| Option | Default | Effect |
| --- | --- | --- |
| `KINETRA_BUILD_RUNTIME` | OFF | Build optional SDL2/OpenGL Runtime |
| `BUILD_TESTING` | ON | Build core tests and external consumer checks |
| `KINETRA_TEST_GRAPHICS` | OFF | Add runtime tests requiring an OpenGL context |

The former Demo, Dotio and Launcher switches have been removed. The root build contains no application executable. Use fresh build directories after migration to avoid stale binaries/options.

## Checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
python3 tools/check_repository.py
```

`kinetra_behavior` and `kinetra_navigation` cover core contracts. Repository checks cover first-party extensions, Markdown TOCs, local links and checker regressions. They do not render Mermaid. Format changed authored C++ using `.clang-format`; leave generated GLAD/KHR unchanged.

## Graphics execution

```sh
cmake -S . -B build-runtime -DKINETRA_BUILD_RUNTIME=ON -DKINETRA_TEST_GRAPHICS=ON
cmake --build build-runtime --parallel
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ctest --test-dir build-runtime --output-on-failure
```

Use a desktop or Xvfb if SDL lacks offscreen support. Software rendering does not establish desktop performance or feel. Application presentation, menu, switching and bounded smoke checks run in [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab#tests-and-resources).

## External consumers

[check_consumer.cmake](../tools/check_consumer.cmake) configures, builds and runs the separate [consumer](../tests/consumer/). It checks public simulation interfaces and, in runtime builds, public runtime headers/linking. Neither fixture reads application sources or requires a window.

```sh
ctest --test-dir build -R kinetra_external --output-on-failure
```

[Installation](../README.md#using-kinetra) exports Simulation and optional Runtime. Lab can consume that package or a sibling source checkout. Binary ABI compatibility is not promised.
