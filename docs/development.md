# Development and verification

Use the root [quick start](../README.md#quick-start) for ordinary builds. This page owns advanced options and verification procedures.

## Table of contents

- [Dependencies and options](#dependencies-and-options)
- [Checks](#checks)
- [Graphics execution](#graphics-execution)
- [External consumers](#external-consumers)
- [Container dependency installation](#container-dependency-installation)

## Dependencies and options

C++20, CMake 3.16+ and a build tool are required. Python 3 runs the repository validation test. Runtime additionally requires SDL2 development files, OpenGL development files and a 3.3-capable context. On Artix/Arch these are supplied by the usual `base-devel`, `cmake`, `sdl2`, `mesa` and `python` packages; installing packages is the developer's environment choice.

| Option | Default | Effect |
| --- | --- | --- |
| `KINETRA_BUILD_DEMO` | ON | Bundled Demo/Dotio launcher and compatibility sandbox |
| `KINETRA_BUILD_RUNTIME` | Initial Demo option value | Independent Runtime target and application presentations |
| `KINETRA_BUILD_LAUNCHER` | Initial Demo option value | Independent Launcher target; requires Runtime |
| `KINETRA_BUILD_DOTIO` | Initial Demo option value | In-tree standalone Dotio; requires Runtime for presentation/executable |
| `BUILD_TESTING` | ON | CTest suites and application logic tests |
| `KINETRA_TEST_GRAPHICS` | OFF | Tests needing a real OpenGL context |

Bundled Demo requires Runtime, Launcher and Dotio enabled. Library-only builds can disable all applications. Options are cached: use fresh directories or set every relevant option explicitly when changing configurations. The root project places runnable binaries directly in `build/`; embedding Kinetra does not overwrite the parent's output-directory setting.

## Checks

```sh
python3 tools/check_repository.py
ctest --test-dir build --output-on-failure
```

The repository check validates first-party extensions, Markdown TOCs and local Markdown links/anchors. Vendor/generated dependency and build directories are excluded. It does not render Mermaid or validate arbitrary prose claims. Review diagram syntax and component names against implementation; report rendering as unverified unless actual tooling was used.

Format changed authored C++ using `.clang-format`; leave generated GLAD/KHR untouched. Run the smallest relevant suite first, then the full configured suite after boundary changes.

| Test | Area |
| --- | --- |
| `kinetra_behavior` / `kinetra_navigation` | Headless engine contracts |
| `kinetra_terrain` / `kinetra_sandbox_application` | Demo policy and orchestration |
| `kinetra_dotio` | Dotio rules, navigation and streaming |
| `kinetra_menu` / `kinetra_launcher` | Bundled choices and generic factories; no window required |
| Names containing `graphics` | Engine/application rendering and integration |
| `kinetra_repository` / `kinetra_repository_checker` | Source/documentation conventions and negative enforcement tests |
| `kinetra_external_headless` / `kinetra_external_dotio` | Separate consumer configure/build/run checks |

## Graphics execution

Enable graphics tests, rebuild, then run on your desktop:

```sh
cmake -S . -B build -DKINETRA_TEST_GRAPHICS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

For SDL builds with an offscreen backend and software Mesa:

```sh
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ctest --test-dir build --output-on-failure
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/kinetra --frames 120
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/kinetra --variant demo --frames 120
SDL_VIDEODRIVER=offscreen LIBGL_ALWAYS_SOFTWARE=1 ./build/kinetra --variant dotio --frames 120
```

Alternatively use `LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a` before each command. Offscreen availability depends on SDL/Mesa configuration. Frame limits bound rendering, not deterministic simulation duration. Software tests do not establish desktop input feel, balance or performance.

## External consumers

CTest runs [check_consumer.cmake](../tools/check_consumer.cmake), which configures a separate [consumer](../tests/consumer/), compiles public math/physics/navigation calls and executes them. The graphics-enabled fixture also copies Dotio to its own `apps/dotio`, builds its standalone executable and links through public Kinetra targets. This deliberately detects sibling/private include assumptions.

```sh
ctest --test-dir build -R kinetra_external --output-on-failure
```

The build boundary requires no display. Running the copied Dotio still requires the graphics environment above. Runtime-only integration is documented in [runtime](engine/runtime.md#external-consumers).

## Container dependency installation

This workspace can use unpacked dependencies under `/tmp/kinetra-deps/root`, rather than system-installed development packages. These paths are environment-specific, not a portable requirement:

```sh
export LD_LIBRARY_PATH=/tmp/kinetra-deps/root/usr/lib/x86_64-linux-gnu:/tmp/kinetra-deps/root/lib/x86_64-linux-gnu:/tmp/kinetra-deps/root/usr/lib/x86_64-linux-gnu/pulseaudio
export SDL_VIDEODRIVER=offscreen
export LIBGL_ALWAYS_SOFTWARE=1
export LIBGL_DRIVERS_PATH=/tmp/kinetra-deps/root/usr/lib/x86_64-linux-gnu/dri
export __EGL_VENDOR_LIBRARY_FILENAMES=/tmp/kinetra-deps/root/usr/share/glvnd/egl_vendor.d/50_mesa.json
export MESA_SHADER_CACHE_DISABLE=true
/tmp/kinetra-deps/root/usr/bin/cmake -S . -B build -DCMAKE_PREFIX_PATH=/tmp/kinetra-deps/root/usr -DKINETRA_BUILD_DEMO=ON -DKINETRA_TEST_GRAPHICS=ON -DCMAKE_BUILD_TYPE=Release \
  "-DCMAKE_CXX_FLAGS=-isystem /tmp/kinetra-deps/root/usr/include -isystem /tmp/kinetra-deps/root/usr/include/x86_64-linux-gnu"
/tmp/kinetra-deps/root/usr/bin/cmake --build build --parallel 4
/tmp/kinetra-deps/root/usr/bin/ctest --test-dir build --output-on-failure
```

The external fixture inherits dependency compiler flags from the parent build. The unpacked Debian SDL headers require their architecture-specific include directory. Include auxiliary library directories when linking SDL/PulseAudio. Missing EGL vendor discovery can produce “Could not find a valid EGL device” even when Mesa is present. Use a writable shader cache or disable it in restricted environments.
