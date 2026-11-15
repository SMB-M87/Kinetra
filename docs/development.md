# Development and verification

Core builds need no Kinetra-Lab checkout. Application builds and tests belong to Lab.

## Table of contents

**Navigation:**

[Directory guide](README.md) · [Project overview](../README.md) · [Documentation index](README.md)

**On this page:**

- [Dependencies and options](#dependencies-and-options)
- [Checks](#checks)
- [Graphics execution](#graphics-execution)
- [External consumers](#external-consumers)

## Dependencies and options

C++20, CMake 3.16+ and a build tool are required. Runtime additionally requires SDL2/OpenGL development files and an OpenGL 3.3 context.

| Option | Default | Effect |
| --- | --- | --- |
| `KINETRA_BUILD_RUNTIME` | OFF | Build optional SDL2/OpenGL Runtime |
| `BUILD_TESTING` | ON | Build core tests |
| `KINETRA_TEST_GRAPHICS` | OFF | Add runtime tests requiring an OpenGL context |

`KINETRA_BUILD_RUNTIME` also builds `Kinetra::Launcher` and its window-free tests. Application build switches belong to Kinetra-Lab. The root build contains no application executable. Use fresh build directories after migration to avoid stale binaries/options.

## Checks

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

`kinetra_behavior` and `kinetra_navigation` cover core contracts. Check changed documentation links and paths when editing docs. Format changed authored C++ using `.clang-format`; leave generated GLAD/KHR unchanged.

## Graphics execution

```sh
cmake -S . -B build-runtime -DKINETRA_BUILD_RUNTIME=ON -DKINETRA_TEST_GRAPHICS=ON
cmake --build build-runtime --parallel
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ctest --test-dir build-runtime --output-on-failure
```

Use a desktop or Xvfb if SDL lacks offscreen support. Software rendering does not establish desktop performance or feel. Application presentation, menu, switching and bounded smoke checks run in [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab#tests-and-resources).

## External consumers

The separate [consumer fixture](../tests/consumer/) checks public simulation interfaces and, with `CONSUMER_GRAPHICS=ON`, runtime headers/linking without opening a window. Run it explicitly when changing the public build boundary:

```sh
cmake -S tests/consumer -B build-consumer -DKINETRA_SOURCE="$PWD"
cmake --build build-consumer --parallel
./build-consumer/consumer
```

For runtime linking, add `-DCONSUMER_GRAPHICS=ON` and provide the SDL2/OpenGL dependencies. Kinetra is integrated from source with `add_subdirectory`; installed CMake packages are not provided.
