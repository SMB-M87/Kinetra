# Development and verification

How to build Kinetra, which options and presets exist, and how to set up the tools around the code: editor, warnings, static analysis and sanitizers. Core builds need no Kinetra-Lab checkout; application builds and tests belong to Lab. Which checks a change needs is decided by its [verification level](testing.md#verification-levels).

## Table of contents

- [Dependencies and options](#dependencies-and-options)
- [Presets](#presets)
- [Checks](#checks)
- [Graphics execution](#graphics-execution)
- [External consumers](#external-consumers)
- [Editor and tools](#editor-and-tools)
- [Open points](#open-points)

[Project overview](../README.md) · [Documentation index](README.md)

## Dependencies and options

Requirements:

- **Always:** C++20 (GCC or Clang), CMake 3.16 or newer, and a build tool.
- **Presets:** CMake 3.25 or newer and Ninja; ccache speeds up rebuilds.
- **Runtime:** SDL2 2.0.18 or newer (on Arch: `sdl2-compat`) and OpenGL development files, plus an OpenGL 3.3 context.

Install them per distribution family. CI tests Arch and Ubuntu 24.04 ([continuous integration](testing.md#continuous-integration)); the pinned Arch toolchain is the reference for warnings and formatting.

| Family | Command | Tested |
| --- | --- | --- |
| Arch, Artix | `sudo pacman -S --needed base-devel cmake ninja ccache clang sdl2-compat libglvnd mesa` | CI's `checks` job, on a pinned snapshot; `clang` provides clang-format |
| Debian, Ubuntu | `sudo apt update && sudo apt install build-essential cmake ninja-build ccache libsdl2-dev libgl-dev libegl-dev libgl1-mesa-dri libegl-mesa0` | CI's `ubuntu` job on Ubuntu 24.04 LTS (GCC 13, glibc 2.39); clang-format is the `clang-format` package, but only the Arch version decides the format |
| Fedora | `sudo dnf install gcc-c++ cmake ninja-build ccache SDL2-devel mesa-libGL-devel mesa-libEGL-devel mesa-dri-drivers` | Not tested, and the package names are not yet checked against Fedora's repositories |

Other distributions need the same parts: a C++20 compiler, CMake, Ninja, SDL2's and OpenGL's development files, and Mesa (or a GPU driver) for an OpenGL 3.3 context.

| Option | Default | Effect |
| --- | --- | --- |
| `KINETRA_BUILD_RUNTIME` | OFF | Build the optional SDL2/OpenGL `Kinetra::Runtime` and `Kinetra::Launcher` |
| `BUILD_TESTING` | ON | Build core tests |
| `KINETRA_TEST_GRAPHICS` | OFF | Add runtime tests that need an OpenGL context (CTest label `graphics`) |
| `KINETRA_WARNINGS` | OFF | Compile Kinetra targets with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion` |
| `KINETRA_WARNINGS_AS_ERRORS` | OFF | Add `-Werror` to that warning set. CI enables it: Kinetra must stay warning-free |

The warning set and test registration live in [cmake/](../cmake/). Kinetra-Lab uses the same helpers (`kinetra_directory_warnings`, `kinetra_add_doctest`) with its own options. Use fresh build directories after changing options to avoid stale binaries.

## Presets

[CMakePresets.json](../CMakePresets.json) bundles the option combinations. Every preset builds in `build-<preset>/`, which is ignored by Git, and exports `compile_commands.json` there for clangd. Every preset except `ci` uses ccache. A workflow preset configures, builds and tests in one command:

```sh
cmake --workflow --preset dev
```

| Preset | Build type | Runtime | Extras | Tests run |
| --- | --- | --- | --- | --- |
| `dev` | Debug | on | warnings | all except `graphics` |
| `release` | Release | on | | all except `graphics` |
| `headless` | Release | off | | all |
| `asan` | Debug | off | AddressSanitizer + UndefinedBehaviorSanitizer | all |
| `graphics` | Release | on | graphics tests, `SDL_VIDEODRIVER=offscreen`, software Mesa | all, including `graphics` |
| `ci` | Release | on | warnings as errors, graphics tests, `SDL_VIDEODRIVER=offscreen`, software Mesa | all, including `graphics` |

`release` has configure, build and test presets but no workflow preset. Run its steps separately: `cmake --preset release && cmake --build --preset release && ctest --preset release`. For iterative work, rebuild one target with `cmake --build --preset dev --target <target>`.

## Checks

```sh
cmake --workflow --preset ci       # CI: everything compiled, warnings as errors
cmake --workflow --preset headless # CI: simulation only, without SDL2/OpenGL
cmake --workflow --preset dev      # day-to-day
```

Without presets (CMake older than 3.25), configure and build directly:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Check changed documentation with `python3 tools/check_docs.py`, and formatting with `clang-format --dry-run --Werror <files>`. Leave generated GLAD/KHR and `third_party/` unchanged.

## Graphics execution

```sh
cmake --workflow --preset graphics
```

The preset sets `SDL_VIDEODRIVER=offscreen` and `LIBGL_ALWAYS_SOFTWARE=1` for the tests, as the `ci` preset does in CI. Use a desktop or Xvfb if SDL lacks offscreen support. Software rendering does not establish desktop performance or feel. Application presentation, menu, switching and bounded smoke checks run in [Kinetra-Lab](https://github.com/SMB-M87/Kinetra-Lab#tests-and-resources).

## External consumers

The separate [consumer fixture](../tests/consumer/) checks the public simulation interfaces. With `CONSUMER_GRAPHICS=ON`, it also checks the runtime headers and linking, without opening a window. Run it when changing the public build boundary (CI always runs it):

```sh
cmake -S tests/consumer -B build-consumer -G Ninja -DKINETRA_SOURCE="$PWD" -DCONSUMER_GRAPHICS=ON
cmake --build build-consumer
./build-consumer/consumer
```

Kinetra is integrated from source with `add_subdirectory`; no installed CMake package is provided.

## Editor and tools

| Tool | Setup | Use |
| --- | --- | --- |
| clangd | [.clangd](../.clangd) points at `build-dev/compile_commands.json`; run `cmake --preset dev` once | Completion, go-to-definition, `///` contracts on hover, diagnostics with the project's flags |
| clang-format | [.clang-format](../.clang-format), clang-format 23 (same as CI) | Format the files you change; most editors do it on save |
| clang-tidy | [.clang-tidy](../.clang-tidy) | `run-clang-tidy -p build-dev <files>` on changed files. Checks: bugprone, performance, selected modernize, and naming per the [conventions](conventions.md#naming). Not enforced in CI yet |
| Sanitizers | `asan` preset | `cmake --workflow --preset asan` finds memory and undefined-behavior errors in the headless code |
| gdb | `dev` preset (Debug) | `gdb --args build-dev/<test executable>` |

`git blame` skips the formatting-only commits listed in [.git-blame-ignore-revs](../.git-blame-ignore-revs) after `git config blame.ignoreRevsFile .git-blame-ignore-revs`; GitHub does this automatically.

## Open points

- clang-tidy findings are not yet triaged; enforcing a subset in CI is a later decision.
