# Contact response tests

Status: implemented · Branch: `test/contact-response-tests` · Related: Kinetra-Lab [Demo restructuring](https://github.com/SMB-M87/Kinetra-Lab/blob/main/docs/design/2026-11-21-demo-restructuring.md) (roadmap phase 5, step 3)

Kinetra-Lab's Demo carries a test suite, `labs/demo/tests/contact_tests.cpp`, that includes only `kinetra/simulation/physics.h`, links only `Kinetra::Simulation` and checks nothing of the Demo. It tests the engine, so it moves here as an engine suite.

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

The six cases of `contact_tests.cpp` (head-on elastic contacts at 60 and 120 Hz, contact geometry under a rigid transform, an off-center impact, thin-wall tunneling, resting contact) guard `World::step`. In Lab they run only when Lab builds, so an engine change can break them without Kinetra's CI noticing.

## Goals and non-goals

**Goals:** the six cases in Kinetra, unchanged, as their own suite with a row in the test map and the targeted-tests table.

**Non-goals:** new cases, or merging them with overlapping cases in `simulation_tests.cpp` and `moving_contact_tests.cpp`.

## Decisions

| Decision | Choice | Why |
| --- | --- | --- |
| Suite | A new suite `kinetra_contact_response_tests` (prefix `kinetra_contact_response`), not part of `simulation_tests.cpp` | The cases form one topic, the response of whole bodies to contact, next to the swept-contact suites; `simulation_tests.cpp` is already the largest suite |
| Content | The cases verbatim; only the file comment changes | A move must not change what is tested; Lab's verification shows the cases are the same |

## Design

`tests/contact_response_tests.cpp` holds the cases. The root `CMakeLists.txt` registers it next to `kinetra_moving_contact_tests`. Lab removes its copy in the same phase step and records this branch's commit.

## Affected areas

`tests/contact_response_tests.cpp`, `CMakeLists.txt`, `tests/README.md`, `docs/testing.md`, this document and the design index. No public header or target changes.

## Test plan

| Behavior | Proof | Level |
| --- | --- | --- |
| The six cases pass in Kinetra | `cmake --build --preset dev --target kinetra_contact_response_tests`, `ctest --preset dev -R '^kinetra_contact_response:'` | Targeted |
| The CMake change builds everything | `cmake --workflow --preset graphics` | Full |

## Review

Self-review: the file is Lab's `contact_tests.cpp` with a new header comment (`diff` against the Lab copy shows only the first two lines). No findings.

## Verification

| Check | Command | Result |
| --- | --- | --- |
| Targeted | `cmake --build --preset dev --target kinetra_contact_response_tests`; `ctest --preset dev -R '^kinetra_contact_response:'` | passed, 6 cases |
| Full | `cmake --workflow --preset graphics` | see the pull request |
| Docs | `python3 tools/check_docs.py` | passed |

## Pull request

Title: `test(physics): move the contact response cases from Kinetra-Lab`

Summary: Kinetra-Lab's Demo tested `World::step` contact response in `labs/demo/tests/contact_tests.cpp` without using anything of the Demo. The six cases move here unchanged as `kinetra_contact_response_tests`, so Kinetra's CI guards them. Lab removes its copy in a pull request that records this branch's commit; this pull request merges first, with a merge commit.

## Open points

- `simulation_tests.cpp` also has an elastic circle collision and an off-center rotation case; whether they and these cases should be merged is left open.
