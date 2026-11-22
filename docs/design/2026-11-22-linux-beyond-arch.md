# Linux beyond Arch

Status: accepted · Branches: `fix/portable-atan2-asin`, `build/linux-beyond-arch` · Related: [roadmap item](../roadmap.md#now), Kinetra-Lab design *Linux beyond Arch and golden tests on every toolchain* (`docs/design/2026-11-22-linux-beyond-arch.md` in Kinetra-Lab)

Kinetra builds and passes its tests on Ubuntu 24.04, but its float angles depend on the C library: `atan2f` and `asinf` round differently in glibc before 2.41 than on CI's Arch toolchain, which moves Kinetra-Lab's golden fingerprints on every other Linux. This stack makes those two functions round the same everywhere, then adds an Ubuntu LTS job to CI and dependency commands per distribution family, so Linux in general is supported and the docs say what is tested.

## Table of contents

- [Problem](#problem)
- [Goals and non-goals](#goals-and-non-goals)
- [Decisions](#decisions)
- [Design](#design)
- [Affected areas](#affected-areas)
- [Test plan](#test-plan)
- [Review](#review)
- [Verification](#verification)
- [Pull request](#pull-request)
- [Open points](#open-points)

[Directory guide](README.md) · [Project overview](../../README.md) · [Documentation index](../README.md)

## Problem

**Only Arch is documented and tested.** [AGENTS.md](../../AGENTS.md#identity-and-scope) names Artix/Linux the desktop target, [development](../development.md#dependencies-and-options) gives only a `pacman` command, and [CI](../../.github/workflows/ci.yml) runs only the pinned Arch image.

**Kinetra itself works on Ubuntu.** Measured on 2026-11-22 in a cloud session on Ubuntu 24.04.5 (GCC 13.3, Clang 18.1, CMake 3.28, glibc 2.39, SDL2 2.30, Mesa 25.2) with `libsdl2-dev`, `libgl-dev` and `ccache` installed: `cmake --workflow --preset headless` passes 228 of 228 tests, `cmake --workflow --preset ci` builds without a warning under `-Werror` and passes 278 of 278 tests, the 13 graphics cases on software OpenGL included, and the external consumer builds and runs.

**The angles of a simulation depend on the C library.** Kinetra-Lab's five trajectory fingerprints (four Demo crowd characterization cases and i4sim's RVO golden case) fail on that Ubuntu session, and its roadmap blamed GCC 13. The compiler is not the cause: GCC 13 Release, GCC 13 Debug and Clang 18 Release give bit-identical, and equally wrong, summaries. The C library is:

- glibc 2.41 replaced `atan2f` and `asinf` (among other float functions) with correctly rounded implementations from the CORE-MATH project. CI's Arch snapshot has a newer glibc; Ubuntu 24.04 has 2.39, Debian 12 has 2.36.
- An `LD_PRELOAD` shim that replaces only `atan2f` and `asinf` with `(float)atan2((double)y, (double)x)` and `(float)asin((double)x)` makes all five Lab cases pass on Ubuntu with the values recorded on Arch, and the rest of Lab (316 cases) and Kinetra (278 cases) stays green. Replacing `atan2f` alone fixes four of the five; also replacing `sinf`, `cosf`, `expf` or `powf` breaks them again, so those functions already agree between the two glibc versions.
- On random inputs, glibc 2.39's `atan2f` and `asinf` differ from the correctly rounded result in the last bit about once in twenty calls; for example `atan2f(0x1.05ebb8p+5, -0x1.ed77fp+3)` gives `0x1.0167e8p+1` where the correctly rounded value is `0x1.0167eap+1`. A chaotic crowd turns one such bit into a different trajectory.

In Kinetra, `std::atan2` on floats is called by `signed_angle` ([math.h](../../include/kinetra/simulation/math.h)) and twice by `tangent_steer`, which also calls `std::asin` ([priority_steering.cpp](../../src/simulation/priority_steering.cpp)).

## Goals and non-goals

Goals:

- `kinetra::atan2` and `kinetra::asin` for floats that give the same, correctly rounded result with every C library Kinetra supports, used by Kinetra's simulation, so that Lab can use them in its applications.
- A CI job on Ubuntu LTS that builds and tests Kinetra next to the pinned Arch job, which stays the reference for warnings and formatting.
- Dependency commands per distribution family (Arch and Artix, Debian and Ubuntu, Fedora), and docs that name what CI tests.

Non-goals:

- Cross-machine bitwise determinism in general. `sinf`, `cosf`, `expf`, `powf` and `hypot` still come from the C library; they agree between the two glibc versions measured, not necessarily with Windows' C runtime. The contract keeps saying that bitwise equality across machines is not promised.
- A Fedora CI job (decided on 2026-11-22: documented, not tested).
- Packaging or a Linux release; that is Lab's *Linux release* item.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| How golden tests hold on every toolchain | Kinetra's own `atan2` and `asin`, computed in double and rounded once to float; the recorded fingerprints stay strict (the user, 2026-11-22) | The measurements show these two functions are the whole difference between glibc versions. A tolerance or totals would weaken every fingerprint, and a fingerprint per toolchain multiplies the values to maintain |
| CI on another distribution | One job on Ubuntu LTS next to the pinned Arch job, which stays the reference for warnings and formatting; Fedora documented, not tested (the user, 2026-11-22) | Ubuntu LTS is GitHub's own runner image and the most common Linux; one more job keeps CI time reasonable |
| Order of the stack | The math first, then the CI job and docs | The math is what Lab needs to start; the CI step changes `.github/` and the agent rules, so it waits for the user, and the bottom step should not wait with it |
| Names | `kinetra::atan2(float, float)` and `kinetra::asin(float)` in `math.h` | Callers write `kinetra::atan2` where they wrote `std::atan2`; inside namespace `kinetra` an unqualified call finds these first. An unqualified call next to `using namespace kinetra;` would be ambiguous, which fails to compile rather than picking the wrong one |
| Warnings on Ubuntu | Reported, not errors | Arch's pinned compiler is the reference; a newer or older GCC may warn about other things, and that should not fail a pull request. Formatting is checked only on Arch, whose clang-format is pinned |

## Design

### Step 1: `fix/portable-atan2-asin`

```cpp
/// Angle of the point (`x`, `y`) in radians, in [-pi, pi], like std::atan2. Computed in double
/// and rounded once to float, so the result is the correctly rounded value on every C library.
inline float atan2(float y, float x) {
    return static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x)));
}
```

and the same for `asin`. *Why double:* a double `atan2` or `asin` is accurate to about one double ulp on every C library in use, 29 bits more than a float holds, so the one rounding to float gives the correctly rounded float result unless the exact value lies within a double ulp of the midpoint between two floats. That double rounding is the one case where two C libraries can still disagree; on 5 million random `atan2` inputs, rounding the double and the 64-bit `long double` result to float never differed. glibc 2.41 and later return exactly these values from `atan2f` and `asinf`, so Arch results stay what they were.

`signed_angle` and `tangent_steer` call them. Cost: a double `atan2` costs about as much as the float one on x86-64; `tangent_steer` calls it per neighbor, which no benchmark shows as a hotspot.

### Step 2: `build/linux-beyond-arch`

- A second job `ubuntu` in `ci.yml` on `ubuntu-24.04`, without a container: `apt-get` installs the packages from the new Debian and Ubuntu command, then the job runs the headless workflow, the `ci` configure preset with `-DKINETRA_WARNINGS_AS_ERRORS=OFF`, its build and its tests (graphics cases on software OpenGL included), and the external consumer. It skips the format, docs and icon checks, which do not depend on the distribution. The required status stays `CI / checks`; making `CI / ubuntu` required is the user's choice in the branch protection.
- [development](../development.md#dependencies-and-options) gets a command per family. The Debian and Ubuntu command is the package set CI installs; the Fedora command is documented and marked untested.
- AGENTS.md, the README and the testing page say which distributions CI tests.

## Affected areas

- Step 1: `include/kinetra/simulation/math.h` (public header), `src/simulation/priority_steering.cpp`, `tests/simulation_tests.cpp`, [math and physics](../engine/math-physics.md), [simulation README](../../src/simulation/README.md#invariants-and-contracts).
- Step 2: `.github/workflows/ci.yml`, `docs/development.md`, `docs/testing.md`, `README.md`, `AGENTS.md`.
- Both: this document, the [design index](README.md#index) and the [roadmap](../roadmap.md).

## Test plan

| Behavior | Test | Level |
| --- | --- | --- |
| `kinetra::atan2` returns the correctly rounded float where glibc 2.39's `atan2f` does not | New case in `tests/simulation_tests.cpp` with two inputs found on Ubuntu 24.04, each pinned to its correctly rounded value; first with `std::atan2`, where it fails on glibc 2.39 | Targeted, then Full |
| The same for `kinetra::asin` | Same case | Targeted, then Full |
| `kinetra::atan2` keeps `std::atan2`'s quadrants, signed zeros and infinities, and `asin` returns NaN outside [-1, 1] | Same case | Targeted |
| `signed_angle` and `tangent_steer` keep their behavior | The existing simulation and steering suites | Targeted |
| Kinetra builds and passes on Ubuntu LTS | The new `ubuntu` CI job | Full (CI) |
| Lab's golden fingerprints hold on Ubuntu with Arch's recorded values | Kinetra-Lab's pull request, with its own Ubuntu job | Full (Lab CI) |

## Review

Filled in when each step is reviewed.

## Verification

Filled in when each step is verified.

## Pull request

Filled in per step.

## Open points

- Windows' C runtime may still differ in `sinf`, `cosf`, `expf`, `powf` or `hypot`; Kinetra-Lab's pull request tries its Windows-skipped golden cases and records what still differs.
