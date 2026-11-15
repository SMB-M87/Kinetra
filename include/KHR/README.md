# Khronos platform definitions

This directory contains the bundled Khronos platform header required by GLAD.

## Table of contents

- [Contents](#contents)
- [Integration and maintenance](#integration-and-maintenance)

[Parent guide](../README.md) · [Project overview](../../README.md) · [Documentation index](../../docs/README.md)

## Contents

[khrplatform.h](khrplatform.h) provides platform-specific types and calling convention definitions for Khronos APIs. Its upstream provenance and license are embedded in the header.

## Integration and maintenance

The generated [GLAD header](../glad/README.md) includes this file. It supports the optional runtime backend rather than defining an application-facing Kinetra API.

Preserve the upstream header and its license. This README explains its role without modifying third-party source. Return to the [header map](../README.md).
