# doctest

Vendored copy of [doctest](https://github.com/doctest/doctest) **v2.5.3**, a single-header C++ test framework (MIT, see [LICENSE.txt](LICENSE.txt)). It is checked in so that builds and CI need no network access or system package.

## Table of contents

- [Contents](#contents)
- [Updating](#updating)

[Parent guide](../../README.md) · [Documentation index](../../docs/README.md) · [Testing](../../docs/testing.md)

## Contents

| Path | Origin | Purpose |
| --- | --- | --- |
| `doctest/doctest.h` | upstream `doctest/doctest.h` (sha256 `cfd518a3…1421`) | The framework |
| `cmake/doctest.cmake`, `cmake/doctestAddTests.cmake` | upstream `scripts/cmake/` | One CTest entry per `TEST_CASE` |
| `LICENSE.txt` | upstream | MIT license |
| `CMakeLists.txt`, `doctest_main.cpp` | Kinetra | `kinetra_doctest` (system include) and `kinetra_doctest_main` (shared `main()`) |

Upstream files are unchanged; do not edit them. Tests are registered with `kinetra_add_doctest` from [KinetraTesting.cmake](../../cmake/KinetraTesting.cmake).

## Updating

Download the same four upstream files from the new release tag, update the version and checksum above, run the full verification level, and note the update in the pull request.
