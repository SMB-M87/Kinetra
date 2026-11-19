# Generated OpenGL loader headers

This directory contains the generated GLAD header used by the optional SDL2/OpenGL runtime.

## Table of contents

- [Contents](#contents)
- [Integration and maintenance](#integration-and-maintenance)

[Project overview](../../README.md) · [Documentation index](../../docs/README.md)

## Contents

[glad.h](glad.h) declares OpenGL types, constants and loaded function pointers. Its generation metadata is embedded at the top of the file. The matching implementation is [src/glad.c](../../src/glad.c).

## Integration and maintenance

The [runtime implementation](../../src/engine/README.md) uses GLAD to access OpenGL entry points. The header includes [KHR platform definitions](../KHR/README.md). These backend details are kept out of the [public Kinetra headers](../kinetra/).

Preserve generated code and embedded licenses. This README documents the bundled files without changing them.
